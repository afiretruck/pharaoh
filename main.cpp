/********************************************************************************
*
* WARNING This file is subject to the terms and conditions defined in the file
* 'LICENSE.txt', which is part of this source code package. Please ensure you
* have read the 'LICENSE.txt' file before using this software.
*
*********************************************************************************/

#include "WindowManager.h"
#include <memory>

using namespace Pharaoh;
using namespace std;

int main(int argc, char** argv)
{
    unique_ptr<WindowManager> xWindowManager;
    xWindowManager.reset(new WindowManager(argc, argv));

    return xWindowManager->Run();
}