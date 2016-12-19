
#include "base/latextools.h"

using namespace insight;

int main(int argc, char*argv[])
{
  SimpleLatex sl(
    "This is a test using chars _#[]^%&~öäü<>.\n"
    "Filenname: /path/to/bla_da_du.stlb\n"
    "A formula $\\alpha+\\beta \\frac{a+b}{c}^n$ within text\n"
    "And a display formula $$\\alpha+\\beta \\frac{a+b}{c}^n$$ within text\n"
    "Image: \\includegraphics[width=\\linewidth]{test}\n"
     "Another Image: \\includegraphics[width=0.75\\linewidth]{ship/xStern}\n"
  );
  
  std::cout<<"=== Latex: ===\n" << sl.toLaTeX()<<"\n"<<std::endl;
  
  std::cout<<"=== HTML: ===\n" << sl.toHTML()<<"\n"<<std::endl;

  std::cout<<"=== Plaintext: ===\n" << sl.toPlainText()<<"\n"<<std::endl;
  
  return 0;
}
