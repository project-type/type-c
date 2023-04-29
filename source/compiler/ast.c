//
// Created by praisethemoon on 28.04.23.
//

#include "ast.h"
#include "../utils/vec.h"

PackageID* ast_makePackageID() {
    PackageID* package = malloc(sizeof(PackageID));
    vec_init(&package->ids);

    return package;
}