#ifndef SDK_FIX_H
#define SDK_FIX_H

// fixes different signature between SDK versions
#define mem_zalloc(s) pvPortZalloc(s, 0, 0)

#endif
