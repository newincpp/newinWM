//http://seasonofcode.com/posts/how-x-window-managers-work-and-how-to-write-one-part-ii.html
#include "WindowManager.hh"

int main() {
    std::unique_ptr<newin::WindowManager> wm(newin::WindowManager::Create());
    wm->run();
}
