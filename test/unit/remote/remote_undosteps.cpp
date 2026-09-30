/*
 * UndoSteps (src/workbench/remoterun.h): rollback of the steps
 * performed while launching a remote run.
 */

#include "remoterun.h"

#include "base/exception.h"

#include "remotetest.h"


using namespace insight;
using namespace insight::remotetest;




class TestableUndoSteps : public UndoSteps
{
public:
    size_t nSteps() const { return undoSteps_.size(); }
};




int main(int argc, char* argv[])
{
    TestRunner tr("remote_undosteps", argc, argv);


    tr.run("undo steps are executed in reverse order", [&]()
    {
        TestableUndoSteps u;
        std::vector<int> order;
        u.addUndoStep([&](){ order.push_back(1); }, "step 1");
        u.addUndoStep([&](){ order.push_back(2); }, "step 2");
        u.addUndoStep([&](){ order.push_back(3); }, "step 3");

        u.performUndo(nullptr, false);

        check(order==std::vector<int>({3,2,1}), "unexpected undo order");
    });


    tr.run("failing undo step does not stop the rollback", [&]()
    {
        TestableUndoSteps u;
        std::vector<int> order;
        u.addUndoStep([&](){ order.push_back(1); }, "step 1");
        u.addUndoStep([&](){ throw insight::Exception("undo of step 2 failed"); }, "step 2");
        u.addUndoStep([&](){ order.push_back(3); }, "step 3");

        u.performUndo(nullptr, false);

        check(order==std::vector<int>({3,1}), "remaining undo steps were not executed");
    });


    tr.run("undo step throwing a non-std exception does not stop the rollback", [&]()
    {
        TestableUndoSteps u;
        std::vector<int> order;
        u.addUndoStep([&](){ order.push_back(1); }, "step 1");
        u.addUndoStep([&](){ throw 42; }, "step 2");

        checkIsolated([&]()
        {
            u.performUndo(nullptr, false);
            check(order==std::vector<int>({1}), "remaining undo step was not executed");
        }, "rollback with non-std exception");
    });


    tr.run("rethrow passes on the original exception", [&]()
    {
        TestableUndoSteps u;
        u.addUndoStep([](){}, "step 1");

        auto reason = std::make_exception_ptr(
            insight::Exception("original reason for the rollback") );

        auto msg = expectThrows<insight::Exception>(
            [&](){ u.performUndo(reason, true); },
            "performUndo with rethrow" );
        check(msg.find("original reason")!=std::string::npos,
              "rethrown exception does not carry the original message: "+msg);
    });


    tr.run("undo steps are consumed and not executed twice", [&]()
    {
        // a remote run may run into several error paths (e.g. error and cancellation)
        TestableUndoSteps u;
        int n=0;
        u.addUndoStep([&](){ ++n; }, "kill remote process");

        u.performUndo(nullptr, false);
        u.performUndo(nullptr, false);

        check(n==1, "undo step was executed %d times", n);
        check(u.nSteps()==0, "undo steps should be cleared after rollback");
    });


    return tr.finish();
}
