 project(code_asterextensions)

install_package_script(Aster __init__.py FSI.py geometry.py post.py)

install_script(isExtractVTKFromRMED isExtractVTKFromRMED.py)
install_script(isRunSalomePvPython isRunSalomePvPython.sh)
