#include "ssestreamparser.h"

namespace insight {
namespace assistant {


void SSEStreamParser::processLine(const QByteArray& line, QList<Event>& events)
{
    if (line.isEmpty())
    {
        // dispatch
        if (hasData_ || !event_.isEmpty())
        {
            events.append({event_, data_});
        }
        event_.clear();
        data_.clear();
        hasData_=false;
        return;
    }

    if (line.startsWith(':'))
        return; // comment

    QByteArray field, value;
    int colon=line.indexOf(':');
    if (colon<0)
    {
        field=line;
    }
    else
    {
        field=line.left(colon);
        value=line.mid(colon+1);
        if (value.startsWith(' '))
            value.remove(0, 1);
    }

    if (field=="event")
    {
        event_=value;
    }
    else if (field=="data")
    {
        if (hasData_)
            data_.append('\n');
        data_.append(value);
        hasData_=true;
    }
    // "id" and "retry" are not used
}


QList<SSEStreamParser::Event> SSEStreamParser::feed(const QByteArray& chunk)
{
    QList<Event> events;
    buffer_.append(chunk);

    int start=0;
    for (;;)
    {
        int nl=buffer_.indexOf('\n', start);
        if (nl<0)
            break;
        QByteArray line=buffer_.mid(start, nl-start);
        if (line.endsWith('\r'))
            line.chop(1);
        processLine(line, events);
        start=nl+1;
    }
    buffer_.remove(0, start);

    return events;
}


QList<SSEStreamParser::Event> SSEStreamParser::finish()
{
    QList<Event> events;
    if (!buffer_.isEmpty())
    {
        QByteArray line=buffer_;
        if (line.endsWith('\r'))
            line.chop(1);
        buffer_.clear();
        processLine(line, events);
    }
    processLine(QByteArray(), events);
    return events;
}


void SSEStreamParser::reset()
{
    buffer_.clear();
    event_.clear();
    data_.clear();
    hasData_=false;
}


} // namespace assistant
} // namespace insight
