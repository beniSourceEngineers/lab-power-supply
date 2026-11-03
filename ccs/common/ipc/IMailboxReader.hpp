#ifndef IPC_IMAILBOXREADER_HPP
#define IPC_IMAILBOXREADER_HPP

namespace ipc {

// Receiving side of a latest-value channel from the other core. Messages that
// are overwritten before they are read are lost; use a queue if each one counts.
template <typename T>
class IMailboxReader {
public:
    // Copies the newest unread message to out and returns true. Returns false
    // and leaves out unchanged if there is none, or if the writer kept
    // overwriting it during the read (it is still unread: try again later).
    // Never blocks for long, so it is safe in an ISR.
    virtual bool tryRead(T& out) = 0;

    virtual bool hasNew() const = 0;

protected:
    ~IMailboxReader() {}
};

} // namespace ipc

#endif
