//
// Created by maciej on 15.08.25.
//

#ifndef RAILGEOMETRYBUILDER_HPP
#define RAILGEOMETRYBUILDER_HPP

struct RailParams
{
    float gauge;        // odstęp szyn od osi czyli krzywej CR
    float railRadius; // średnica szyny
    int ringSides = 10;
    bool closeLoop = false;
};

class RailGeometryBuilder {

};



#endif //RAILGEOMETRYBUILDER_HPP
