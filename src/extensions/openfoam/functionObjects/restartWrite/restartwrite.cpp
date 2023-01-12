
#include "functionObject.H"
#include "addToRunTimeSelectionTable.H"
#if defined(OF_FORK_extend) && OF_VERSION>010601 //(defined(Fx40)||defined(Fx41)||defined(Fx32))
#include "foamTime.H"
#else
#include "Time.H"
#endif

#if OF_VERSION>020100
#include "fileOperation.H"
#endif

#include "uniof.h"

namespace Foam
{



class restartWrite
: public UniFunctionObject
{
private:

    // Private data

        const Time& time_;

        mutable FIFOStack<word> previousWriteTimes_;

        int skippedSteps_;
        scalar lastWriteTime_;
        scalar clockTimeInterval_;

        label nKeep_;

    // Private Member Functions

        //- Remove write flag file.
        void removeFile() const;

        //- Disallow default bitwise copy construct
        restartWrite(const restartWrite&);

        //- Disallow default bitwise assignment
        void operator=(const restartWrite&);


public:

    //- Runtime type information
    TypeName("restartWrite");


    // Constructors

        //- Construct for given objectRegistry and dictionary.
        restartWrite
        (
            const word& name,
            const Time&,
            const dictionary&
        );


    // Member Functions

        //- Read the dictionary settings
        bool read(const dictionary&) override;

        //- Execute, check existence of abort file and take action
        bool perform() override;

        //- Execute, check existence of abort file and take action
        bool write() override;
};




defineTypeNameAndDebug(restartWrite, 0);
addToRunTimeSelectionTable
(
    functionObject,
    restartWrite,
    dictionary
);




restartWrite::restartWrite
(
    const word& name,
    const Time& time,
    const dictionary& dict
)
:
  UniFunctionObject(name, dict),
    time_(time),
    skippedSteps_(0),
    lastWriteTime_(time_.elapsedClockTime())
{}




// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool restartWrite::read(const dictionary& dict)
{
    clockTimeInterval_ = dict.lookupOrDefault<scalar>("clockTimeInterval", 600);
    nKeep_ = dict.lookupOrDefault<label>("nKeep", 2);
    Info
          <<"Writing restart output every "<<clockTimeInterval_
          <<" seconds wall clock time, keeping last "
          <<nKeep_<<" outputs."<<endl;
    return true;
}


bool restartWrite::perform()
{
    const int nskip =
#if (OF_VERSION<=020100) && !(defined(OF_FORK_extend) && OF_VERSION>=010604)
            0
#else
            1
#endif
    ;

    // check write
    scalar now = 0;
    if (Pstream::master())
    {
        now = time_.elapsedClockTime();
    }
    reduce(now, sumOp<scalar>());

    if ( now - lastWriteTime_ > clockTimeInterval_ )
    {

        if (skippedSteps_==0)
        {
#if (OF_VERSION<=020100) && !(defined(OF_FORK_extend) && OF_VERSION>=010604)
            const_cast<Time&>(time_).writeNow();
#else
            const_cast<Time&>(time_).writeOnce(); // writes after time increase
#endif
        }

        if (skippedSteps_<nskip) // timeName, outputTime etc. match only in next step
        {
            skippedSteps_++;
        }
        else
        {
#warning overlap with regular write cannot be checked due to delayed writing
//            if (!time_.outputTime()) // only take action, if it had not been due anyway
            {
                Info<< "RESTART DATA WRITE AT (timeIndex="
                    << time_.timeIndex()<<", timeName = "
                    << time_.timeName()
                    << ")"
                    << endl;

                previousWriteTimes_.push(time_.timeName());

                // Purge last, if required
                while (previousWriteTimes_.size() > nKeep_)
                {
#if OF_VERSION<=020100
                    auto fn =
                            time_.objectRegistry::path(
                                previousWriteTimes_.pop() );
                    if (isDir(fn))
                    {
                        rmDir(fn);
                    }
#else
                    auto fn = time_.objectRegistry::path(
                                    previousWriteTimes_.pop() );
                    if (isDir(fn))
                    {
                        fileHandler().rmDir( fn );
                    }
#endif
                }
            }


            skippedSteps_ = 0;
            lastWriteTime_ = now;
        }

    }

    return true;
}




bool restartWrite::write()
{
   return true;
}



}
