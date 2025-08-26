//
// Created by maciej on 06.08.25.
//

#ifndef DRAWABLEMIXIN_HPP
#define DRAWABLEMIXIN_HPP


template <typename T>
class DrawableMixin {
public:
    void drawImpl() {
        static_cast<T*>(this)->draw();
    }
};


#endif // DRAWABLEMIXIN_HPP
