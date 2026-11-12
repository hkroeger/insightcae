#ifndef INSIGHT_ASSISTANT_SSESTREAMPARSER_H
#define INSIGHT_ASSISTANT_SSESTREAMPARSER_H

#include <QByteArray>
#include <QList>

namespace insight {
namespace assistant {


/**
 * incremental parser for server-sent events (text/event-stream)
 */
class SSEStreamParser
{
public:
    struct Event
    {
        QByteArray event;
        QByteArray data;
    };

private:
    QByteArray buffer_;
    QByteArray event_;
    QByteArray data_;
    bool hasData_ = false;

    void processLine(const QByteArray& line, QList<Event>& events);

public:
    /**
     * add received data, return all events, which are complete now
     */
    QList<Event> feed(const QByteArray& chunk);

    /**
     * the stream has ended: return a pending event, which was not terminated by an empty line
     */
    QList<Event> finish();

    void reset();
};


} // namespace assistant
} // namespace insight

#endif // INSIGHT_ASSISTANT_SSESTREAMPARSER_H
