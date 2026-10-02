/*
 * insight::AnalyzeClient (src/toolkit_remote) against a fake analyze REST server
 * with injected faults.
 */

#include <condition_variable>
#include <iostream>
#include <mutex>

#include "analyzeclient.h"
#include "base/resultset.h"
#include "base/remotecommand.h"

#include "fakeanalyzeserver.h"
#include "remotetest.h"


using namespace insight;
using namespace insight::remotetest;
using namespace std::chrono_literals;

typedef FakeAnalyzeServer::Fault Fault;




/**
 * records all calls from the client
 */
class RecordingProgressDisplayer : public ProgressDisplayer
{
public:
    mutable std::mutex mx;
    std::vector<ProgressState> states;
    std::vector<std::string> logLines;
    std::vector<std::pair<std::string,double> > progressValues;
    std::vector<std::pair<std::string,std::string> > messageTexts;
    std::vector<std::string> finishedActions;

    void setActionProgressValue(const std::string &path, double value) override
    {
        std::lock_guard<std::mutex> l(mx);
        progressValues.push_back({path, value});
    }
    void setMessageText(const std::string &path, const std::string& message) override
    {
        std::lock_guard<std::mutex> l(mx);
        messageTexts.push_back({path, message});
    }
    void finishActionProgress(const std::string &path) override
    {
        std::lock_guard<std::mutex> l(mx);
        finishedActions.push_back(path);
    }
    void reset() override
    {}
    void update ( const ProgressState& pi ) override
    {
        std::lock_guard<std::mutex> l(mx);
        states.push_back(pi);
    }
    void logMessage(const std::string& line) override
    {
        std::lock_guard<std::mutex> l(mx);
        logLines.push_back(line);
    }
};




/**
 * collects the outcome of an asynchronous client call
 */
template<class R>
struct Outcome
{
    std::mutex mx;
    std::condition_variable cv;
    int nResults=0, nTimeouts=0;
    std::unique_ptr<R> result;

    std::function<void(R)> resultCallback()
    {
        return [this](R r)
        {
            std::lock_guard<std::mutex> l(mx);
            nResults++;
            result=std::make_unique<R>(std::move(r));
            cv.notify_all();
        };
    }

    std::function<void()> timeoutCallback()
    {
        return [this]()
        {
            std::lock_guard<std::mutex> l(mx);
            nTimeouts++;
            cv.notify_all();
        };
    }

    bool waitForAny(std::chrono::milliseconds t)
    {
        std::unique_lock<std::mutex> l(mx);
        return cv.wait_for(l, t, [this]{ return nResults+nTimeouts>0; });
    }

    bool waitForResult(std::chrono::milliseconds t)
    {
        std::unique_lock<std::mutex> l(mx);
        return cv.wait_for(l, t, [this]{ return nResults>0; });
    }

    bool waitForTimeout(std::chrono::milliseconds t)
    {
        std::unique_lock<std::mutex> l(mx);
        return cv.wait_for(l, t, [this]{ return nTimeouts>0; });
    }
};

typedef Outcome<QueryStatusAction::Result> StatusOutcome;
typedef Outcome<QueryResultsAction::Result> ResultsOutcome;
typedef Outcome<AnalyzeClientAction::ReportSuccessResult> ControlOutcome;




std::string sampleResultsXml()
{
    ResultSet rs(nullptr, "remote test results", "subtitle");
    std::ostringstream os;
    rs.saveToStream(os, hierarchicalData::Element::OutputProperties());
    return os.str();
}




int main(int argc, char* argv[])
{
    TestRunner tr("remote_analyzeclient", argc, argv);


    tr.run("status query is parsed and forwarded to the progress displayer", [&]()
    {
        FakeAnalyzeServer srv;
        FakeAnalyzeServer::Status st;
        st.states.push_back({1.5, {{"residual/p", 1e-3}, {"residual/U", 2e-3}}, "iteration 1"});
        st.logLines={"log line 1", "log line 2"};
        FakeAnalyzeServer::Status::ProgressState ps1; ps1.path="setup"; ps1.hasValue=true; ps1.value=0.5;
        FakeAnalyzeServer::Status::ProgressState ps2; ps2.path="setup"; ps2.hasText=true; ps2.text="meshing";
        FakeAnalyzeServer::Status::ProgressState ps3; ps3.path="setup";
        st.progressStates={ps1, ps2, ps3};
        srv.setStatus(st);

        RecordingProgressDisplayer pd;
        AnalyzeClient ac("test", srv.url(), &pd);

        StatusOutcome o;
        ac.queryStatus(o.resultCallback(), o.timeoutCallback());
        check(o.waitForResult(10s), "no status result received");

        check(o.result->success, "status query not successful");
        check(!o.result->resultsAreAvailable, "results should not be available");
        check(!o.result->errorOccurred, "no error expected");

        std::lock_guard<std::mutex> l(pd.mx);
        check(pd.states.size()==1, "expected one progress state, got %d", int(pd.states.size()));
        check(pd.states[0].first==1.5, "progress state time");
        check(pd.states[0].second.at("residual/p")==1e-3, "progress variable");
        check(pd.states[0].logMessage_=="iteration 1", "progress state log message");
        check(pd.logLines.size()==2 && pd.logLines[1]=="log line 2", "log lines");
        check(pd.progressValues.size()==1 && pd.progressValues[0].second==0.5, "action progress value");
        check(pd.messageTexts.size()==1 && pd.messageTexts[0].second=="meshing", "action message text");
        check(pd.finishedActions.size()==1, "finished action");

        auto reqs = srv.requests();
        check(reqs.size()==1 && reqs[0].method=="GET" && reqs[0].path=="/all",
              "expected GET /all request");
    });


    tr.run("results availability is reported", [&]()
    {
        FakeAnalyzeServer srv;
        FakeAnalyzeServer::Status st;
        st.resultsAvailable=true;
        srv.setStatus(st);

        AnalyzeClient ac("test", srv.url(), nullptr);
        StatusOutcome o;
        ac.queryStatus(o.resultCallback(), o.timeoutCallback());
        check(o.waitForResult(10s), "no status result received");
        check(o.result->success && o.result->resultsAreAvailable, "results should be available");
    });


    tr.run("analysis error on the server is reported", [&]()
    {
        FakeAnalyzeServer srv;
        FakeAnalyzeServer::Status st;
        st.errorOccurred=true;
        st.errorMessage="You wanted this error.";
        st.errorStackTrace="frame1\nframe2";
        srv.setStatus(st);

        AnalyzeClient ac("test", srv.url(), nullptr);
        StatusOutcome o;
        ac.queryStatus(o.resultCallback(), o.timeoutCallback());
        check(o.waitForResult(10s), "no status result received");
        check(o.result->errorOccurred, "error should be reported");
        check(bool(o.result->exception), "exception object missing");
        std::string msg = o.result->exception->message();
        check(msg.find("You wanted this error.")!=std::string::npos,
              "error message not transferred: "+msg);
    });


    tr.run("server error message with printf format characters is transferred verbatim", [&]()
    {
        // the error message must not be interpreted as a format string
        checkIsolated([]()
        {
            FakeAnalyzeServer srv;
            FakeAnalyzeServer::Status st;
            st.errorOccurred=true;
            st.errorMessage="solver diverged at 100% of %s steps (%d)";
            st.errorStackTrace="trace";
            srv.setStatus(st);

            AnalyzeClient ac("test", srv.url(), nullptr);
            StatusOutcome o;
            ac.queryStatus(o.resultCallback(), o.timeoutCallback());
            check(o.waitForResult(10s), "no status result received");
            check(bool(o.result->exception), "exception object missing");
            std::string msg = o.result->exception->message();
            check(msg=="solver diverged at 100% of %s steps (%d)",
                  "error message was altered: "+msg);
        }, "transfer of error message with format characters", 30s);
    });


    tr.run("results are fetched", [&]()
    {
        FakeAnalyzeServer srv;
        srv.setResults(sampleResultsXml());

        AnalyzeClient ac("test", srv.url(), nullptr);
        ResultsOutcome o;
        ac.queryResults(o.resultCallback(), o.timeoutCallback());
        check(o.waitForResult(10s), "no results callback received");
        check(o.result->success, "results query not successful");
        check(bool(o.result->results), "no result set returned");
        check(o.result->results->title()=="remote test results",
              "unexpected result title: "+o.result->results->title());
    });


    tr.run("HTTP error while fetching results is reported", [&]()
    {
        FakeAnalyzeServer srv;
        srv.setResults(sampleResultsXml());
        srv.setFault(Fault::HttpError);

        AnalyzeClient ac("test", srv.url(), nullptr);
        ResultsOutcome o;
        ac.queryResults(o.resultCallback(), o.timeoutCallback());
        check(o.waitForAny(10s), "no callback received");
        check(o.nResults==1, "result callback expected");
        check(!o.result->success, "query should not be successful");
        check(!o.result->results, "no results expected");
    });


    tr.run("unreachable server is reported quickly", [&]()
    {
        int freePort = findFreePort();
        AnalyzeClient ac("test", "http://127.0.0.1:"+std::to_string(freePort), nullptr);
        StatusOutcome o;
        ac.queryStatus(o.resultCallback(), o.timeoutCallback());
        check(o.waitForAny(10s), "no callback within 10 s for an unreachable server");
        check(o.nResults==1 ? !o.result->success : true, "query should not be successful");
    });


    tr.run("timeout callback is called, if the server does not respond", [&]()
    {
        FakeAnalyzeServer srv;
        srv.setFault(Fault::NeverRespond);

        auto o = std::make_shared<StatusOutcome>();
        {
            AnalyzeClient ac("test", srv.url(), nullptr);
            ac.setTimeout(2s);
            ac.queryStatus(o->resultCallback(), o->timeoutCallback());

            bool gotTimeout = o->waitForTimeout(8s);
            check(gotTimeout,
                  "onTimeout was not called within 8 s (request timeout 2 s); "
                  "result callbacks: %d", o->nResults);
        }
    });


    tr.run("timeout callback is not called after a successful request", [&]()
    {
        FakeAnalyzeServer srv;
        AnalyzeClient ac("test", srv.url(), nullptr);
        ac.setTimeout(1s);

        StatusOutcome o;
        ac.queryStatus(o.resultCallback(), o.timeoutCallback());
        check(o.waitForResult(10s), "no status result received");
        std::this_thread::sleep_for(2s);
        check(o.nTimeouts==0, "onTimeout called after successful request");
        check(o.nResults==1, "result callback called %d times", o.nResults);
    });


    tr.run("delayed response within timeout is accepted", [&]()
    {
        FakeAnalyzeServer srv;
        srv.setFault(Fault::DelayResponse, 1000);
        AnalyzeClient ac("test", srv.url(), nullptr);
        ac.setTimeout(5s);

        StatusOutcome o;
        ac.queryStatus(o.resultCallback(), o.timeoutCallback());
        check(o.waitForResult(10s), "no status result received");
        check(o.result->success, "delayed status query should succeed");
        check(o.nTimeouts==0, "no timeout expected");
    });


    tr.run("connection closed without response is reported", [&]()
    {
        FakeAnalyzeServer srv;
        srv.setFault(Fault::CloseConnection);
        AnalyzeClient ac("test", srv.url(), nullptr);

        StatusOutcome o;
        ac.queryStatus(o.resultCallback(), o.timeoutCallback());
        check(o.waitForAny(10s), "no callback received");
        check(o.nResults==1 && !o.result->success, "failure expected");
    });


    tr.run("response without content type is reported as failure", [&]()
    {
        checkIsolated([]()
        {
            FakeAnalyzeServer srv;
            srv.setFault(Fault::NoContentType);
            AnalyzeClient ac("test", srv.url(), nullptr);

            StatusOutcome o;
            ac.queryStatus(o.resultCallback(), o.timeoutCallback());
            check(o.waitForAny(10s), "no callback received");
            check(o.nResults==1 && !o.result->success, "failure expected");
        }, "status response without content type", 30s);
    });


    tr.run("results response without content type is reported as failure", [&]()
    {
        checkIsolated([]()
        {
            FakeAnalyzeServer srv;
            srv.setResults(sampleResultsXml());
            srv.setFault(Fault::NoContentType);
            AnalyzeClient ac("test", srv.url(), nullptr);

            ResultsOutcome o;
            ac.queryResults(o.resultCallback(), o.timeoutCallback());
            check(o.waitForAny(10s), "no callback received");
            check(o.nResults==1 && !o.result->success, "failure expected");
        }, "results response without content type", 30s);
    });


    tr.run("malformed JSON is reported as failure", [&]()
    {
        checkIsolated([]()
        {
            FakeAnalyzeServer srv;
            srv.setFault(Fault::MalformedJson);
            AnalyzeClient ac("test", srv.url(), nullptr);

            StatusOutcome o;
            ac.queryStatus(o.resultCallback(), o.timeoutCallback());
            check(o.waitForAny(10s), "no callback received");
            check(o.nResults==1 && !o.result->success, "failure expected");
        }, "malformed JSON status response", 30s);
    });


    tr.run("unexpected content type is reported as failure", [&]()
    {
        FakeAnalyzeServer srv;
        srv.setHandler([](const FakeAnalyzeServer::Request&)
        {
            FakeAnalyzeServer::Response r;
            r.contentType="text/html";
            r.body="<html>proxy error page</html>";
            return r;
        });
        AnalyzeClient ac("test", srv.url(), nullptr);

        StatusOutcome o;
        ac.queryStatus(o.resultCallback(), o.timeoutCallback());
        check(o.waitForAny(10s), "no callback received");
        check(o.nResults==1 && !o.result->success, "failure expected");
    });


    tr.run("control requests send the expected payload", [&]()
    {
        FakeAnalyzeServer srv;
        AnalyzeClient ac("test", srv.url(), nullptr);

        std::vector<std::pair<std::string, std::function<void(AnalyzeClientAction::ReportSuccessCallback, AnalyzeClientAction::SimpleCallBack)> > > actions =
        {
            {"kill", [&](auto a, auto b){ ac.kill(a, b); }},
            {"wnow", [&](auto a, auto b){ ac.wnow(a, b); }},
            {"wnowandstop", [&](auto a, auto b){ ac.wnowandstop(a, b); }},
            {"exit", [&](auto a, auto b){ ac.exit(a, b); }}
        };

        for (auto& a: actions)
        {
            ControlOutcome o;
            a.second(o.resultCallback(), o.timeoutCallback());
            check(o.waitForResult(10s), "no response to "+a.first);
            check(o.result->success, a.first+" not successful");

            auto r = srv.requests().back();
            check(r.method=="POST", a.first+": expected POST");
            check(r.contentType=="application/json", a.first+": content type "+r.contentType);
            check(r.body.find("\""+a.first+"\"")!=std::string::npos,
                  a.first+": unexpected body "+r.body);
        }
    });


    tr.run("execution path query", [&]()
    {
        FakeAnalyzeServer srv;
        AnalyzeClient ac("test", srv.url(), nullptr);
        Outcome<QueryExepathAction::Result> o;
        ac.queryExepath(o.resultCallback(), o.timeoutCallback());
        check(o.waitForResult(10s), "no response");
        check(o.result->success, "not successful");
        check(o.result->exePath=="/tmp/fake-exepath", "exe path: "+o.result->exePath.string());
    });


    tr.run("sequential requests reuse the client", [&]()
    {
        FakeAnalyzeServer srv;
        AnalyzeClient ac("test", srv.url(), nullptr);
        for (int i=0; i<20; ++i)
        {
            StatusOutcome o;
            ac.queryStatus(o.resultCallback(), o.timeoutCallback());
            check(o.waitForResult(10s), "no response to request %d", i);
            check(o.result->success, "request %d not successful", i);
        }
        check(srv.requestCount()==20, "server received %d requests", int(srv.requestCount()));
    });


    tr.run("request issued while another one is pending is not lost", [&]()
    {
        // e.g. "write now" is requested from the GUI while the status is being monitored
        checkIsolated([]()
        {
            FakeAnalyzeServer srv;
            srv.setFault(Fault::DelayResponse, 1000);
            AnalyzeClient ac("test", srv.url(), nullptr);

            StatusOutcome o1;
            ControlOutcome o2;
            ac.queryStatus(o1.resultCallback(), o1.timeoutCallback());
            ac.wnow(o2.resultCallback(), o2.timeoutCallback());

            check(o1.waitForAny(10s), "no response to first request");
            check(o2.waitForAny(10s), "no response to second request");
            check(o1.nResults==1 && o1.result->success, "first request failed");
            check(o2.nResults==1 && o2.result->success, "second request failed");
        }, "concurrent requests", 30s);
    });


    tr.run("client can be destroyed while a request is pending", [&]()
    {
        checkIsolated([]()
        {
            FakeAnalyzeServer srv;
            srv.setFault(Fault::NeverRespond);
            {
                AnalyzeClient ac("test", srv.url(), nullptr);
                StatusOutcome o;
                ac.queryStatus(o.resultCallback(), o.timeoutCallback());
                std::this_thread::sleep_for(500ms);
            }
        }, "destroying client with pending request", 20s);
    });


    tr.run("client can be destroyed while delayed callbacks are scheduled", [&]()
    {
        checkIsolated([]()
        {
            FakeAnalyzeServer srv;
            AnalyzeClient ac("test", srv.url(), nullptr);
            ac.ioService().schedule(10s, [](){});
        }, "destroying client with scheduled work", 20s);
    });


    return tr.finish();
}
