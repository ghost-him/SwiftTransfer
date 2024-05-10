//
// Created by ghost-him on 24-4-28.
//

#include "IDGenerator.h"

uint32_t IDGenerator::getUniqueID() {
    uint32_t id = _id ++;
    return id;
}
