#include <iostream>

int main()
{
    // "new" should not be highlighted as a keyword
    int new = 0;
    for (;;) { }
    // "true" and "false" are C23 keywords, so it should be highlighted
    while (true) { }
    if (false) { }
    // "nullptr" is also C23
    void* p = nullptr;
    _BitInt(32) x = 0;
}
