#ifndef NUCLEAR_REACTION_H
#define NUCLEAR_REACTION_H
#include <functional>
namespace NUClear {
    class Reaction {
        public:
            Reaction(std::function<void ()> callback);
            ~Reaction();

            void operator()();
        private:
            std::function<void ()> m_callback;
    };
}
#endif
