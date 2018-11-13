/*=========================================================================
  =                             Includes                                  =
  =========================================================================*/

#include <event.h>
#include <mama/mama.h>
#include <mama/io.h>
#include <mama/integration/bridge/base.h>
#include "basedefs.h"
#include <wombat/port.h>
#include <mama/integration/io.h>

/*=========================================================================
  =                Typedefs, structs, enums and globals                   =
  =========================================================================*/

typedef struct baseIoEventImpl
{
    uint32_t            mDescriptor;
    mamaIoCb            mAction;
    mamaIoType          mIoType;
    mamaIo              mParent;
    void*               mClosure;
    struct event        mEvent;
} baseIoEventImpl;

#ifndef evutil_socket_t
#define evutil_socket_t int
#endif


/*=========================================================================
  =                  Private implementation prototypes                    =
  =========================================================================*/

void*
baseBridgeMamaIoImpl_dispatchThread (void* closure);

void
baseBridgeMamaIoImpl_libeventIoCallback (evutil_socket_t fd, short type, void* closure);

baseBridgeClosure*
baseBridgeMamaIoImpl_getQpidBridgeClosure (baseIoEventImpl* impl);


/*=========================================================================
  =                   Public implementation functions                     =
  =========================================================================*/

mama_status
baseBridgeMamaIo_create         (ioBridge*   result,
                                 void*       nativeQueueHandle,
                                 uint32_t    descriptor,
                                 mamaIoCb    action,
                                 mamaIoType  ioType,
                                 mamaIo      parent,
                                 void*       closure)
{
    baseIoEventImpl*    impl            = NULL;
    short               evtType         = 0;
    baseBridgeClosure*  bridgeClosure   = NULL;

    if (NULL == result)
    {
        return MAMA_STATUS_NULL_ARG;
    }

    *result = 0;

    /* Check for supported types so we don't prematurely allocate */
    switch (ioType)
    {
    case MAMA_IO_READ:
        evtType = EV_READ;
        break;
    case MAMA_IO_WRITE:
        evtType = EV_WRITE;
        break;
    case MAMA_IO_ERROR:
        evtType = EV_READ | EV_WRITE;
        break;
    case MAMA_IO_CONNECT:
    case MAMA_IO_ACCEPT:
    case MAMA_IO_CLOSE:
    case MAMA_IO_EXCEPT:
    default:
        return MAMA_STATUS_UNSUPPORTED_IO_TYPE;
        break;
    }

    impl = (baseIoEventImpl*) calloc (1, sizeof (baseIoEventImpl));
    if (NULL == impl)
    {
        return MAMA_STATUS_NOMEM;
    }

    impl->mDescriptor           = descriptor;
    impl->mAction               = action;
    impl->mIoType               = ioType;
    impl->mParent               = parent;
    impl->mClosure              = closure;

    event_set (&impl->mEvent,
               impl->mDescriptor,
               evtType,
               baseBridgeMamaIoImpl_libeventIoCallback,
               impl);

    event_add (&impl->mEvent, NULL);

    bridgeClosure = baseBridgeMamaIoImpl_getQpidBridgeClosure(impl);

    event_base_set (bridgeClosure->mIoState.mEventBase, &impl->mEvent);

    /* If this is the first event since base was emptied or created */
    if (0 == bridgeClosure->mIoState.mEventsRegistered)
    {
        wsem_post (&bridgeClosure->mIoState.mResumeDispatching);
    }
    bridgeClosure->mIoState.mEventsRegistered++;

    *result = (ioBridge)impl;

    return MAMA_STATUS_OK;
}

mama_status
baseBridgeMamaIo_destroy        (ioBridge io)
{
    baseIoEventImpl*    impl            = (baseIoEventImpl*) io;
    baseBridgeClosure*  bridgeClosure   = NULL;
    if (NULL == io)
    {
        return MAMA_STATUS_NULL_ARG;
    }
    bridgeClosure = baseBridgeMamaIoImpl_getQpidBridgeClosure(impl);
    event_del (&impl->mEvent);

    free (impl);
    bridgeClosure->mIoState.mEventsRegistered--;

    return MAMA_STATUS_OK;
}

mama_status
baseBridgeMamaIo_getDescriptor  (ioBridge    io,
                                 uint32_t*   result)
{
    baseIoEventImpl* impl = (baseIoEventImpl*) io;
    if (NULL == io || NULL == result)
    {
        return MAMA_STATUS_NULL_ARG;
    }

    *result = impl->mDescriptor;

    return MAMA_STATUS_OK;
}

/*=========================================================================
  =                  Public implementation prototypes                     =
  =========================================================================*/

mama_status
baseBridgeMamaIoImpl_start (void* closure)
{
    int threadResult                        = 0;
    baseBridgeClosure* bridgeClosure        = (baseBridgeClosure*) closure;

    bridgeClosure->mIoState.mEventsRegistered   = 0;
    bridgeClosure->mIoState.mActive             = 1;
    bridgeClosure->mIoState.mEventBase          = event_init ();

    wsem_init (&bridgeClosure->mIoState.mResumeDispatching, 0, 0);
    threadResult = wthread_create (&bridgeClosure->mIoState.mDispatchThread,
                                   NULL,
                                   baseBridgeMamaIoImpl_dispatchThread,
                                   bridgeClosure);
    if (0 != threadResult)
    {
        mama_log (MAMA_LOG_LEVEL_ERROR, "baseBridgeMamaIoImpl_initialize(): "
                  "wthread_create returned %d", threadResult);
        return MAMA_STATUS_PLATFORM;
    }
    return MAMA_STATUS_OK;
}

mama_status
baseBridgeMamaIoImpl_stop (void* closure)
{
    baseBridgeClosure* bridgeClosure        = (baseBridgeClosure*) closure;
    bridgeClosure->mIoState.mActive = 0;

    /* Alert the semaphore so the dispatch loop can exit */
    wsem_post (&bridgeClosure->mIoState.mResumeDispatching);

    /* Flush until event base is empty */
    while (0 == event_base_loop(bridgeClosure->mIoState.mEventBase, EVLOOP_ONCE));

    /* Join with the dispatch thread - it should exit shortly */
    wthread_join (bridgeClosure->mIoState.mDispatchThread, NULL);
    wsem_destroy (&bridgeClosure->mIoState.mResumeDispatching);

    /* Free the main event base */
    event_base_free (bridgeClosure->mIoState.mEventBase);

    return MAMA_STATUS_OK;
}



/*=========================================================================
  =                  Private implementation prototypes                    =
  =========================================================================*/

void*
baseBridgeMamaIoImpl_dispatchThread (void* closure)
{
    int                 dispatchResult  = 0;
    baseBridgeClosure*  bridgeClosure   = (baseBridgeClosure*) closure;

    /* Wait on the first event to register before starting dispatching */
    wsem_wait (&bridgeClosure->mIoState.mResumeDispatching);

    while (0 != bridgeClosure->mIoState.mActive)
    {
        dispatchResult = event_base_loop (bridgeClosure->mIoState.mEventBase,
                                          EVLOOP_NONBLOCK | EVLOOP_ONCE);

        /* If no events are currently registered */
        if (1 == dispatchResult)
        {
            /* Wait until they are */
            bridgeClosure->mIoState.mEventsRegistered = 0;
            wsem_wait (&bridgeClosure->mIoState.mResumeDispatching);
        }
    }
    return NULL;
}

void
baseBridgeMamaIoImpl_libeventIoCallback (evutil_socket_t fd, short type, void* closure)
{
    baseIoEventImpl* impl = (baseIoEventImpl*) closure;

    /* Timeout is the only error detectable with libevent */
    if (EV_TIMEOUT == type)
    {
        /* If this is an error IO type, fire the callback */
        if (impl->mIoType == MAMA_IO_ERROR && NULL != impl->mAction)
        {
            (impl->mAction)(impl->mParent, impl->mIoType, impl->mClosure);
        }
        /* If this is not an error IO type, do nothing */
        else
        {
            return;
        }
    }

    /* Call the action callback if defined */
    if (NULL != impl->mAction)
    {
        (impl->mAction)(impl->mParent, impl->mIoType, impl->mClosure);
    }

    /* Enqueue for the next time */
    event_add (&impl->mEvent, NULL);
}

baseBridgeClosure*
baseBridgeMamaIoImpl_getQpidBridgeClosure(baseIoEventImpl* impl)
{
    mamaBridge          mamaBridgeImpl  = NULL;
    baseBridgeClosure*  bridgeClosure   = NULL;

    mamaIoImpl_getMamaBridge(impl->mParent, &mamaBridgeImpl);
    mamaBridgeImpl_getClosure(mamaBridgeImpl, (void**)&bridgeClosure);

    return bridgeClosure;

}
