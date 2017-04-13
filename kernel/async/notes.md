https://www4.cs.fau.de/Lehre/WS15/V_CS/Vorlesung/folien/handout/10-guases.pdf


biggest issue with delegation in our first DelegationMonitor: a thread accumulates more responsibilities because the response is sent to the client object but the delegation mechanism processes it on the server's thread. all subsequently enqueued requests also handle their response processing on the same server thread instead of pushing the response task back to the client's original thread. Therefore a DelegationMonitor should remember which thread is owning the exclusive access based on the request queue and use this thread for procssing incoming responses. This way, sending a respons does not acquire more responsibility on the server's thread.

second issue: the enqueue operations are optimised for throughput on contention whereas this might not be the typical situation in a well parallised application. An alternative implementation that focuses on a short path for the uncontended case would be nice. Of course this would slow down the enqueuing on locked monitors. Fortunately this would happen in parallel with the current owner's processing and thus might not be visible in practise.



## slide 9: guarded sections

The guard pushed the task ("postlude") to a queue and a separate sequencer is responsible for the delayed processing.

## slide 12--13: enqueue and dequeue (lock-free)

Maintains a single list queue with a pointer to the tail, and each item points to the next newer item. The newest item at the tail contains a null pointer. The queue is emptied from the head on in FIFO order.

Concurrent enqueues are fixed by the slower enqueuer. However, lines 6--7 smell like they need to be an atomic exchange operation.

## slide 14: enqueue and dequeue (wait-free)

GCC's test_and_set primitive got it's name from Intel's instruction and is an atomic exchange in reality.

enqueue: The tail is replaced by a pointer to the new item and then the next pointer of the old tail is updated to the new item. In the time inbetween, a dequeue operation would not see the new item but detect that it has not reached the tail because the tail points to a different item.

dequeue: The head is a dummy item that provides the initial tail. If the head item is linked to another item, the other is removed from the list by updating head->next to head->next->next. However, head->next->next might not yet have been updated. If head->next->next is still a null pointer, assume that head->next is the tail and hence the last item of the chain. Thus, try to replace the tail by compare_exchange to the dummy head. If this is successful, the chain is emptied. Otherwise, we have to wait until the actual head->next->next pointer becomes visible. The CAS in line 14 looks like an mistake, because it is not waiting for anything.

## slide 20: generalisation to guarded sections

This looks very similar to our delegation queue semantics: In unoccupied state the guard directly exeutes the request, otherwise the request is enqueued. On exit from the critical section, the leaving process becomes the sequencer and processes the enqueued requests sequentially.

A future variable (or promise) is used to store the response information and signal the future's consumer.

## slide 41: task allocation

Ah, this is dynamic allocation :)

