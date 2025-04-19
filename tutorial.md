## Variable Declaration
To declara a primitive type variable you do:
var::type = value;
For composite types:
vector::Array<int>[2] = {3, 5};
matrix::Array<Array<int>>[2][2] = {{3, 5}, {3, 5}};
OBS:All initialized variable must be assigned

## Condicions
if(expression){
    do something...
}

## Loops
while(expression){
    do something...
}

for(var::int = 0;var < 23; var+=1){
    do something...
}

## Functions
func functionName(arg1::int, arg2::int){
    do something...
    ret {expression};
}