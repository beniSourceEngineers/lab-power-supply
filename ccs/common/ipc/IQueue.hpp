#ifndef IPC_IQUEUE_HPP
#define IPC_IQUEUE_HPP

namespace ipc {

// FIFO channel to the other core for events and commands, where each element
// counts. One core only pushes, the other only pops.
template <typename T>
class IQueue {
public:
    // Returns false if the queue is full; the element is not stored.
    virtual bool push(const T& element) = 0;

    // Returns false and leaves out unchanged if the queue is empty.
    virtual bool pop(T& out) = 0;

protected:
    ~IQueue() {}
};

} // namespace ipc

#endif
