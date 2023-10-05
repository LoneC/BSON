# BS-less Object Notation
## General purpose file to store information
### Overview
- Supports integers, decimals, and strings.
- All integers are stored as `long long` and all decimals are stored as `double`
- Essential stores all the data in a big hash map.
- Nested 'objects' are supported.
- Currently, arrays of 'objects' are not.
### Bare-bones example
```
cool = 13
num = 37
```
### Texture data example
```
texture {
    file = "mytexture.png"
    grid {
        rows  = 16
        cols  =  8
        cellw = 48
        cellh = 48
    }
}
```
### Array example
```
shopping-list {
    fruits     = [ "Bananas", "Strawberries", "Raisins", "Oranges", "Cherries", "Kiwis", "Saigon" ]
    vegetables = [ "Brocoli", "Snap peas", "Radishes", "Ubes", "Leeks", "Eggplant", "Stokes" ]
    wood-work  = [ "Boards", "Screws", "Inshave", "Screwdriver" ]
    health     = [ "Colgate", "Orthotics", "Ointment", "Laxatives" ]
}
integers = [ 1, 2, 3, 4, 5, 6, 7, 8 ]
decimals = [ -1.3, 45.6, 89.0, -22.0, 3.14159 ]
dollars = 3
coins = 50
```
## Using BSON
```
#include <stdio.h> /* For printing */
#include <bson.h>  /* All you need */
...
BSON      *bson        = bson_open("shoppinglist.bson");
double    *decimals    = bson_int(bson, "decimals");
size_t     decimalslen = bson_len(decimals);
char      *fruits      = bson_str(bson, "shopping-list.fruits");
size_t     fruitslen   = bson_len(fruits);
long long *dollars     = bson_int(bson, "dollars");

printf("%lld\n", *dollars);
size_t i;
for(i = 0; i < decimalslen; i++)
    printf("%-8.3lf", decimals[i]);
putchar('\n');
for(i = 0; i < fruitslen; i++)
    printf("%s\n", fruits[i]);
...
bson_free(&bson);
```
## Read TODO.md!!
### Dependencies
- GCC or Clang
- C Standard Library


