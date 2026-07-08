#include "matplotlibrenderer.h"

#include "base/resultelements/chart.h"
#include "base/exception.h"

#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <armadillo>

#include <fstream>
#include <sstream>
#include <string>


namespace insight {


// Escape a string for embedding in a Python single-quoted string literal.
// Replaces \ with \\ and ' with \'.
static std::string pyEscape(const std::string& s)
{
    std::string r;
    r.reserve(s.size());
    for (char c : s) {
        if (c == '\\') r += "\\\\";
        else if (c == '\'') r += "\\'";
        else r += c;
    }
    return r;
}


// Map a PlotCurveStyle dashType to a matplotlib linestyle string.
static std::string dashTypeToLinestyle(int dt)
{
    switch (dt) {
        case 0:  // gnuplot dt 0 = solid (same as 1)
        case 1:  return "solid";
        case 2:  return "dashed";
        case 3:  return "dotted";
        case 4:  return "dashdot";
        default: return "solid";
    }
}


// Parse the gnuplot "with ..." style from a plotcmd string.
// Returns {withLines, withPoints, withErrors}.
static std::tuple<bool,bool,bool> parseWithStyle(const std::string& cmd)
{
    // Check for combined linespoints first
    if (cmd.find("linespoints") != std::string::npos ||
        cmd.find(" lp")        != std::string::npos)
        return {true, true, false};

    if (cmd.find("errorbars") != std::string::npos ||
        cmd.find("errorlines") != std::string::npos ||
        cmd.find(" e ")       != std::string::npos  ||
        cmd.find(" e\0")      != std::string::npos)
        return {true, false, true};

    if (cmd.find("lines") != std::string::npos ||
        cmd.find(" l ")   != std::string::npos  ||
        cmd.find(" l\0")  != std::string::npos)
        return {true, false, false};

    if (cmd.find("points") != std::string::npos ||
        cmd.find(" p ")    != std::string::npos  ||
        cmd.find(" p\0")   != std::string::npos)
        return {false, true, false};

    return {true, false, false}; // default: lines
}


MatplotlibRenderer::MatplotlibRenderer(const ChartData* data)
  : ChartRenderer(),
    chartData_(data)
{}


void MatplotlibRenderer::render(const boost::filesystem::path& outimagepath) const
{
    // ------------------------------------------------------------------ script
    std::ostringstream py;

    py << "import matplotlib\n"
          "matplotlib.use('Agg')\n"
          "import matplotlib.pyplot as plt\n"
          "import numpy as np\n"
          "\n";

    // Decide if we need a secondary y-axis
    bool needsTwinx = false;
    for (const PlotCurve& pc : chartData_->plc_)
        if (pc.style_.ax_y_ == 2) { needsTwinx = true; break; }

    py << "fig, ax1 = plt.subplots(figsize=(10.0, 7.5))\n";
    if (needsTwinx)
        py << "ax2 = ax1.twinx()\n";

    py << "ax1.set_xlabel(r'" << pyEscape(chartData_->xlabel_) << "', fontsize=12)\n"
       << "ax1.set_ylabel(r'" << pyEscape(chartData_->ylabel_) << "', fontsize=12)\n"
       << "ax1.grid(True)\n"
       << "\n";

    // ---------------------------------------------------------- plot each curve
    int autoColor = 0; // index into matplotlib's default CN cycle

    for (int k = 0; k < static_cast<int>(chartData_->plc_.size()); ++k)
    {
        const PlotCurve& pc = chartData_->plc_[k];

        // Skip formula-only curves (no data points)
        if (pc.xy_.n_rows == 0)
            continue;

        const bool useStyle = pc.plotcmd_.empty(); // PlotCurveStyle constructor was used

        // --- determine drawing style ---
        bool withLines, withPoints, withErrors;
        if (useStyle) {
            withLines  = pc.style_.withLines_;
            withPoints = pc.style_.withPoints_;
            withErrors = pc.style_.errorLines_;
        } else {
            std::tie(withLines, withPoints, withErrors) = parseWithStyle(pc.plotcmd_);
        }
        // Fall back to lines if nothing is set
        if (!withLines && !withPoints && !withErrors)
            withLines = true;

        // --- determine color ---
        std::string colorExpr;
        if (!useStyle || pc.style_.color_ < 0) {
            colorExpr = "f'C{" + std::to_string(autoColor++) + "}'";
        } else {
            // Map gnuplot color index to a fixed color string
            static const char* gnuplotColors[] = {
                "#000000", // 0 black
                "#FF0000", // 1 red
                "#00C000", // 2 green
                "#0000FF", // 3 blue
                "#FF00C0", // 4 magenta
                "#00E6FF", // 5 cyan
                "#C8C800", // 6 yellow
                "#FFA000", // 7 orange
                "#800000", // 8 dark-red
            };
            int ci = pc.style_.color_;
            if (ci < 9)
                colorExpr = "'" + std::string(gnuplotColors[ci]) + "'";
            else
                colorExpr = "f'C{" + std::to_string(ci) + "}'";
        }

        // --- determine linestyle and linewidth ---
        std::string ls = dashTypeToLinestyle(pc.style_.dashType_);
        int lw = std::max(1, pc.style_.lineWidth_);
        std::string marker = withPoints ? "'o'" : "'None'";

        // --- determine label ---
        std::string label;
        if (!pc.style_.title_.empty())
            label = pc.style_.title_;
        else
            label = pc.title(); // extracts t '...' from plotcmd_
        if (label.empty())
            label = pc.plaintextlabel_;

        // --- which axis ---
        std::string ax = (needsTwinx && pc.style_.ax_y_ == 2) ? "ax2" : "ax1";

        // --- emit x / y data arrays ---
        py << "x" << k << " = np.array([";
        for (arma::uword i = 0; i < pc.xy_.n_rows; ++i)
            py << (i ? "," : "") << pc.xy_(i, 0);
        py << "])\n";

        py << "y" << k << " = np.array([";
        for (arma::uword i = 0; i < pc.xy_.n_rows; ++i)
            py << (i ? "," : "") << pc.xy_(i, 1);
        py << "])\n";

        // --- emit the plot call ---
        if (withErrors && pc.xy_.n_cols >= 3) {
            // error bars: column 2 = symmetric error, or cols 2&3 = asymmetric
            py << "ye" << k << " = np.array([";
            for (arma::uword i = 0; i < pc.xy_.n_rows; ++i)
                py << (i ? "," : "") << pc.xy_(i, 2);
            py << "])\n";

            py << ax << ".errorbar(x" << k << ", y" << k << ", yerr=ye" << k
               << ", color=" << colorExpr
               << ", linestyle='" << ls << "'"
               << ", linewidth=" << lw
               << ", label=r'" << pyEscape(label) << "'"
               << ")\n";
        } else {
            py << ax << ".plot(x" << k << ", y" << k
               << ", color=" << colorExpr
               << ", linestyle='" << ls << "'"
               << ", linewidth=" << lw
               << ", marker=" << marker
               << ", label=r'" << pyEscape(label) << "'"
               << ")\n";
        }

    }

    // ----------------------------------------------------- include_zero
    if (chartData_->plc_.include_zero) {
        py << "ylim1 = ax1.get_ylim()\n"
              "if ylim1[0] > 0:\n"
              "    ax1.set_ylim(bottom=0)\n"
              "elif ylim1[1] < 0:\n"
              "    ax1.set_ylim(top=0)\n";
    }

    // ----------------------------------------------------- legend
    py << "\n"
          "handles1, labels1 = ax1.get_legend_handles_labels()\n";
    if (needsTwinx) {
        py << "handles2, labels2 = ax2.get_legend_handles_labels()\n"
              "all_handles = handles1 + handles2\n"
              "all_labels  = labels1  + labels2\n";
    } else {
        py << "all_handles = handles1\n"
              "all_labels  = labels1\n";
    }
    py << "if any(l for l in all_labels):\n"
          "    ax1.legend(all_handles, all_labels)\n"
          "\n";

    // ----------------------------------------------------- save
    py << "plt.tight_layout()\n"
       << "plt.savefig(r'" << pyEscape(boost::filesystem::absolute(outimagepath).string()) << "', dpi=150)\n"
       << "plt.close(fig)\n";

    // ------------------------------------------------ write & run temp script
    boost::filesystem::path scriptPath =
        boost::filesystem::temp_directory_path()
        / boost::filesystem::unique_path("insight-chart-%%%%-%%%%-%%%%.py");

    {
        std::ofstream f(scriptPath.string());
        if (!f)
            throw insight::Exception("MatplotlibRenderer: cannot write temp script to %s",
                                     scriptPath.c_str());
        f << py.str();
    }

    namespace bproc = boost::process;

    auto pyexe=bproc::search_path("python3");
    if (auto*exe=getenv("INSIGHT_MATPLOTLIB_PYTHON"))
    {
        pyexe=exe;
    }

    bproc::child c(
        pyexe,
        bproc::args({scriptPath.string()}),
        bproc::std_out > stdout,
        bproc::std_err > stderr
    );
    c.wait();

    boost::filesystem::remove(scriptPath);

    if (c.exit_code() != 0)
        throw insight::Exception(
            "MatplotlibRenderer: python3 exited with code %d", c.exit_code());
}


} // namespace insight
