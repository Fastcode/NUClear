#ifndef NUCLEAR_OPTIONS_H
#define NUCLEAR_OPTIONS_H

namespace NUClear {
    
    enum EPriority {
        REALTIME,
        HIGH,
        DEFAULT,
        LOW
    };
    
    namespace Internal {
        
        template <enum EPriority P>
        class Priority { Priority() = delete; ~Priority() = delete; };
        
        template <typename TSync>
        class Sync { Sync() = delete; ~Sync() = delete; };
        
        class Single { Single() = delete; ~Single() = delete; };
    }
}

#endif
