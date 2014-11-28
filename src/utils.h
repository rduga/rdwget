#ifndef UTILS_H
#define	UTILS_H

#include "defaults.h"

/**
 * Calculates length of array.
 * (defined as size of array / size of first item of array
 */
#define	length(X) sizeof (X) / sizeof (X[0])

#endif /* UTILS_H */
