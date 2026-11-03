#ifndef IPC_IMAILBOXWRITER_HPP
#define IPC_IMAILBOXWRITER_HPP

namespace ipc {

// Sending side of a latest-value channel to the other core.
template <typename T>
class IMailboxWriter {
public:
    // Replaces the current message. Never blocks.
    virtual void post(const T& msg) = 0;

protected:
    ~IMailboxWriter() {}
};

} // namespace ipc

#endif
