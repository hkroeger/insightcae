#ifndef INSIGHT_LSDYNAINPUTCARD_H
#define INSIGHT_LSDYNAINPUTCARD_H

#include <iostream>
#include <memory>
#include "base/boost_include.h"

namespace insight {




class LSDynaInputDeck;




class LSDynaInputCard
{
    const LSDynaInputDeck *inputDeck_;
public:
    inline void setInputDeck(const LSDynaInputDeck *inputDeck)
    {
        inputDeck_=inputDeck;
    }

    const LSDynaInputDeck &inputDeck() const;

    virtual void write(std::ostream& os) const =0;
};

typedef std::shared_ptr<LSDynaInputCard> LSDynaInputCardPtr;

std::ostream &operator<<(std::ostream& os, const LSDynaInputCard& ic);





namespace LSDynaInputCards {




class InputCardWithId
        : public LSDynaInputCard
{
protected:
    int id_;

public:
    InputCardWithId(int id);
    inline int id() const { return id_; }
};




class IncludeKey
        : public LSDynaInputCard,
        public boost::filesystem::path
{
public:
    IncludeKey(const boost::filesystem::path& fp);
    void write(std::ostream& os) const override;
};


}

} // namespace insight

#endif // INSIGHT_LSDYNAINPUTCARD_H
