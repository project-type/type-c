![](https://raw.githubusercontent.com/project-type/type-c/master/assets/logo-white-bg.png)

# Type-C

Type-C is a programming language that comes with the following features:

- Pattern matching allows for elegant and concise control flow structures, allowing users to match against complex patterns in their code. This includes matching against values, types, and structures, as well as destructuring.

- Algebraic data types allow for the definition of custom data types with multiple possible values. This includes sum types (enums), product types (structs), and the ability to nest these types for more complex data structures.

- We also added support for generics, type inference, and operator overloading, making `type-c` more expressive and flexible.

- For FFI, we added the ability to call C functions using `extern "C"`, as well as the `unsafe` keyword for situations where memory safety cannot be guaranteed.

- Additionally, we added support for default function arguments, named function arguments, and variadic arguments specifically for use with FFI.

- We also made sure to incorporate concurrency through the use of the `async` and `await` keywords, as well as through the standard library's support for multithreading.

- Finally, we made sure to include a robust standard library with common data types, algorithms, and utilities.


# Table of content

1. Import statements

2. Basic data types, operations and literals

3. `let` statement and mutability

4. Control-flow

5. Function declaration and function generics

6. Data types and and Algebraic data types and Pattern Matching

7. Classes, interfaces and generics

8. Advanced types composition

9. Concurrency and Asynchronous behavior

10. Foreign Function Interface (FFI)


# 1. Import statements

In type-c, imports are very similar to python.

For example:

```rust
from opengl import device as dvc
from opengl.renderer import SoftwareRenderer
import opengl.backend 


from qt import Button as Btn, Window as Win, helpers
```

Generally speaking, import statements follows the following pseudo form:

```bnf
<import_stmts> ::= <import_stmt> ('\n' <import_stmt>)*;
<import_stmt> ::= 'import' <id> ('.' <id>)*
                | 'from' ('.' <id>)* 'import' <id> ('.' <id>)* ('as' <id>)?;
```

Obviously import is not a mandatory statement in a program, but it must always be the first thing. It cannot exist after any other kind of statements (obviously comments to do count).

# 2. Basic data types and operations

## 2.1. Primitive Data Types

type-c comes with built-in support for basic data types

| Name | Explication | Range |
| --- | --- | --- |
| `u8` | 8-bit wide unsigned integer | 0 to 255 |
| `i8` | 8-bit wide signed integer | -128 to 127 |
| `u16` | 8-bit wide unsigned integer | 0 to 65,535 |
| `i16` | 16-bit wide signed integer | -32768 to 32767 |
| `u32` | 32-bit wide unsigned integer | 0 to 4,294,967,295 |
| `i32` | 32-bit wide signed integer | –2,147,483,648 to 2,147,483,647 |
| `u64` | 64-bit wide unsigned integer | 0 to 18,446,744,073,709,551,615 |
| `i64` | 64-bit wide signed integer | –9,223,372,036,854,775,808 to 9,223,372,036,854,775,807 |
| `f32` | 32-bit wide floating point number | 3.4E +/- 38 (7 digits) |
| `f64` | 64-bit wide floating point number | 1.7E +/- 308 (15 digits) |
| `bool` | Boolean value (8-bit sized) | `true` or `false` |
| `ptr<T>` | Pointer, generic by nature. | `64bits` on x64 arch else `32bits` |
| `void` | Usually a return type for functions that returns nothing |     |

## 2.2. Operations

type-c operations sorted by precedence.

| Precedence | Operator | Type | Associativity | Overloadable |
| --- | --- | --- | --- | --- |
| 15  | `()` | Function call or call overload | LTR | Yes |
| 15  | `[]` | Array subscription or subscription overload | LTR | Yes |
| 15  | `.` | Member selection | LTR | No  |
| 14  | `++` | Unary post-increment | LTR | Yes |
| 14  | `--` | Unary post-decrement | LTR | Yes |
| 13  | `++` | Unary pre-increment | RTL | Yes |
| 13  | `--` | Unary pre-decrement | RTL | Yes |
| 13  | `+` | Unary plus | RTL | Yes |
| 13  | `-` | Unary minus | RTL | Yes |
| 13  | `!` | Unary logical negation | RTL | Yes |
| 13  | `~` | Unary bitwise complement | RTL | Yes |
| 12  | `*` | Multiplication | LTR | Yes |
| 12  | `/` | Division | LTR | Yes |
| 12  | `%` | Modulus | LTR | Yes |
| 11  | `+` | Addition | LTR | Yes |
| 11  | `-` | Subtraction | LTR | Yes |
| 10  | `<<` | Bitwise left shift | LTR | Yes |
| 10  | `>>` | Bitwise right | LTR | Yes |
| 9   | `<` | Relational less than | LTR | Yes |
| 9   | `<=` | Relational less than or equal | LTR | Yes |
| 9   | `>` | Relational greater than | LTR | Yes |
| 9   | `>=` | Relational greater than or equal | LTR | Yes |
| 9   | `is` | Type comparison (objects only) | LTR | No  |
| 9   | `as` | Type casting | LTR | No  |
| 8   | `==` | Relational is equal to | LTR | No  |
| 8   | `!=` | Relational is not equal to | LTR | No  |
| 7   | `&` | Bitwise AND | LTR | Yes |
| 6   | `^` | Bitwise exclusive OR | LTR | Yes |
| 5   | `\\|` | Bitwise inclusive OR | LTR | Yes |
| 4   | `&&` | Logical AND | LTR | Yes |
| 3   | `\\| | `   | Logical OR | Logical OR |
| 2   | `if .. {} else {}` | Ternary conditional | RTL | No  |
| 1   | `=` | Assignment | RTL | No  |
| 1   | `+=` | Addition assignment | RTL | No  |
| 1   | `-=` | Subtraction assignment | RTL | No  |
| 1   | `*=` | Multiplication assignment | RTL | No  |
| 1   | `/=` | Division assignment | RTL | No  |
| 1   | `%=` | Modulus assignment | RTL | No  |

## 2.3. Literals

### 2.3.a. Integers

| Regex | Semantic | Example |
| --- | --- | --- |
| `0[b\\|B][0-1]+` | Base 2 (Bin) numeric | `0b1001` |
| `0o[0-7]+` | Base 8 (Oct) numeric | `0o5647` |
| `[1-9][0-9]*` | Base 10 (Dec) numeric | `5464987` |
| `0[x\\|X][0-9a-fA-F]+` | Base 16 (Hex) numeric | `0xffeEA325845` |

### 2.3.b. Floats/Doubles

Floating literal can only decimals. The regex used to validate floating points is the following: `[+|-]?(?:[0-9]*)(?:.[0-9]+)(?:[e|E]-[0-9]+)?f?`
The final `f` character is optional, and it indicates that the literal is of type float (f32) and not double (f64; by default)

### 2.3.c. Boolean

Boolean literal are either `true` or `false`.

### 2.3.d. Strings

type-c strings are by default UTF-8. Strings are not basic data types because every string is being transformed into objects. There is no difference between single quoted strings or double quoted strings.

Examples:

```rust
let w: String = "world"
let str1 = 'hello, '+w

let x: u32 = "hello".length
```

# 3. let statement and mutability

In type-c, variables are actually not "variables" per se. They are immutable by default.

Lets start with the basics.

### 3.1. Variable declaration

A variable is declated through the `let` keyword.

```rust
let x: u32 = 1
x = 2 // Invalid! Will throw compile time error.

let mut y = 1 as u32
y = iChangedMyMind() + 1 // this is valid


let x = !true // automatic type inference
let y: u32 = 1 if !x else 0 // same logic here.
```

### 3.2. Let binding

`let` can be used as expression or nested within other `let`. This is valid As long as the variables declared inside nested lets are **immutable** and **have values**.

For example:

```rust
let x: f32 = 1 + let
    y: f32 = get_some_value() in
       log(y * 2.0f) + 1.0f
```

In this sample the first `let` is a declaration `statement`, the second inner `let` is an expression, it declares a variable `y`, whos scope is limited within `in` block after and its value is used to compute a new value which is the final result of the inner `let`.

## 4. Control-flow

### 4.1. if-else Statements

In type-c, if-else conditions do not need to be surrounded by parenthesis. However the body needs to be surrounded by braces.

```rust
let x: i32 = get_some_int_value()
let mut y: i32 = 1;

if x > 0 {
    y = -x
}
else if x < 0 {
    y = x
}
else { // x == 0
    y = -1
}
```

### 4.2. match-case Statements:

```rust
let x: i32 = get_some_int_value()
let mut y: i32 = 1;

match x {
    0 {
        y = -1
    }
    1 {
        y = 1
    }
    _ {
        y = -x
    }
}
```

### 4.3. foreach .. in loops

The `for .. in` loop works with every `Iterable` object. Iterable is an advanced topic and outside the scope of this manual. If the iterator variable is not defined, you can define it in the for statement. Make sure the variable you use in iterations is mutable.

```rust
let my_array = [1, 2, 3]
let max =  my_array[0]

for let mut i in my_array {
    if i > max {
        max = i
    }
}
```

or

```rust
let my_array = [1, 2, 3]
let max =  my_array[0]
let mut i: u32 = 0

for i in my_array {
    if my_array[i] > max {
        max = i
    }
}
```

or you can use classicial `for(;;)` format

```rust
let my_array = [1, 2, 3]
let max =  my_array[0]

for (let mut i = 0; i < my_array.length; i++) {
    if my_array[i] > max {
        max = my_array[i]
    }
}
```

### 4.4. while loop

Very similar to other language:

```rust
let mut c: str = "no"

while c != "yes" {
    print("Will you marry me?")
    c = get_input_from_user()
    // this is how we did bois.
}
```

### 4.5. do .. while loop

```rust
let mut c: str = "no"

do {
    print("Will you marry me?")
    c = get_input_from_user()
} while c != "yes"
```

You can also use `break` and `continue` during any loop.

## 5. Function declaration and function generics

Declaring functions is done through `fn` keyword. functions in type-c are first-class citizens. Functions can be passed as arguments and returned from other functions just like variables.

Functions can also be stored as variables. Functions have their own data types as well, lets start with simple cases

### 5.1. Function declaration

Simple functions that return values can be easily designed following this syntax:

```
'fn' <id> '(' <args>*  <args-with-default-values>* ')' ('->' <type>)? = <expr> 
```

or if the function is long, its better to separate its body into statements

```
'fn' <id> '(' <args>*  <args-with-default-values>* ')' ('->' <type>)? '{' <fn-body>'}'
```

```rust
fn abs(x: i32) = if x > 0 {x} else {-x} // we can automatically guess the type here
fn abs(x: i32) -> i32 {
    if x > 0 {
        return x    
    }
    return -x
}
fn abs(x: i32) -> u32 = x if x > 0 else -x // allows you to specify another type that is comptatible with original
```

### 5.2. Named arguments

We decided not to implement this feature.

### 5.3. mutable arguments

By default the compiler will complain if you attempt to directly override the value of an argument in the body of a function, unless that argument has the `mut` keyword. For example:

```rust
fn do_something(x: u32, mut y: u32) {
    x = 1 // Error.
    y = 1 // Valid
}
```

### 5.4. First class functions

```rust
let x = [3, 2, 1]
map(x, fn(e: i32) = e*4)

let choices = [fn(e: u8) = e*4, fn(e: u8) = e*3, fn(e: u8) = e*1]

fn get_choice(index: u32) => choices[index]

let random_transformer = get_choice(random_int(0, 2))


let y = random_transformer()(3.14)


let z = [1, 2, 3]
map(z, fn(e: u32) = e*2)

let func: fn(e: i32) -> i32 = choices[0]
```

When you store functions in variables, pass them as argument or functions returns, they are treated as anonymous functions

```rust
let x: fn(u32) -> u32
```

### 5.5. Generic functions

```rust
let x = [3, 2, 1]
map(x, fn(e: int) = e*4)


fn map_elt<T>(e: T[], map_fn: fn(T) -> T) = fn apply() {
    return map(e, map_fn)
}
// similar to 
fn map_elt<T>(e: T[], map_fn: fn(T) -> T) = fn apply() = map(e, map_fn)
```

## 6. Data types and and Algebraic data types and Pattern Matching

### 6.1. Regular Data types

In type-c you can define simple data type (equivalent to c-structs) as follows:

```rust
type Point = { x: f32, y: f32 }
```

Also these types can be generic

```
type Box<T> = {
  value: T,
  randomFn: fn(args: T[]) -> T
}
```

### 6.2. Advanced data types

You can create algebraic data types with constructs as follows

```rust
type Option<T> = { Some(value: T), None }

type Result<T, E> = {
  Ok(T),
  Err(E),
}


fn divide(x: f64, y: f64) -> Result<f64, String> {
  if y == 0.0 {
    return Result.Err("division by zero");
  } else {
    return Result.Ok(x / y);
  }
}

// or even better
fn divide(x: f64, y: f64) -> Result<f64, String> = 
        Result.Err("division by zero") if y == 0.0 
        else Result.Ok(x / y)
```

### 6.3. Pattern matching

```rust
type Result<T, E> = { Ok(value: T), Err(error: E) }

/ A function that returns a Result
fn divide(x: f64, y: f64) -> Result<f64, String> {
    if y == 0.0 {
        return Result.Err("division by zero");
    } else {
        return Result.Ok(x / y);
    }
}

// Usage of the divide function
let result = divide(4.0, 2.0);
match result {
    Ok(value) {
        print("Result: {}", value);
    }
    Err(error) {
        print("Error: {}", error);
    }
}
```

More complex example:

```rust
let result: Result<Result<u32, String>, String> = Ok(Ok(42));

match result {
    Ok(Ok(value)) => println!("Success: {}", value),
    Ok(Err(error)) => println!("Inner error: {}", error),
    Err(error) => println!("Outer error: {}", error),
}
```

## 7. Classes, interfaces and generics

### 7.1. Classes

Classes and interface heavily inspired from Java, but classes cannot extend each other.

```rust
type Rectangle = class {    
  let width: f64
  let height: f64

  fn init(width: f64, height: f64) {
    self.width = width
    self.height = height
  }

  fn area() -> f64 {
    return self.width * self.height
  }
}

total_area = (new Rectangle(50, 30)).area()
print("Total area: ", total_area)
```

### 7.2. Interfaces

Similar to Java, interfaces in type-c can extend other interfaces.

```rust
interface Printable {
  print() -> void
}

type Person = class(Printable) {
  let name: string
  let age: i32

  fn init(name: string, age: i32) {
    self.name = name
    self.age = age
  }

  fn print() {
    println("Name: " + self.name)
    println("Age: " + str(self.age))
  }
}

type Book = class(Printable) {
  let title: string = ""
  let author: string = ""

  fn init(title: string, author: string) {
    self.title = title
    self.author = author
  }

  print() {
    println("Title: " + self.title)
    println("Author: " + self.author)
  }
}

let person = Person("Alice", 30)
let book = Book("The Great Gatsby", "F. Scott Fitzgerald")

let items: Printable[] = [person, book]

foreach item in items {
    item.print()
}
```

More examples:

```rust
type Movable = interface  {
    can_move() -> bool
    set_position(x: i32, y: i32, z: i32)
}

type Actor = class(Movable) {
    fn init () {
        self.x = 0
        self.y = 0
        self.z = 0

        print("created")
    }

    fn can_move() = true // or {return true}
    // or
    fn can_move() -> bool = true
    fn can_move() -> bool { return true }
    fn can_move() { return true }

    fn set_position(x: i32, y: i32, z: 32) {
        self.x = x
        self.y = y
        self.z = z       
    }
}
```

### 7.3. Generics

Similar to function generics, classes and interfaces can have Generics as well.

```java
class DynamicArray<T> extends Container<T> implements Iterable<T>, Sortable<T> {
    fn init (data: T[]) {
    ...
    }
}
```

## 8. Advanced types composition

As your code grows, so will your structure. Inspired by typescript, you can compose new datatypes from existing ones, for example:

```java
type UserProfile = struct {
    age: f32, name: String
}


type UserBody = struct {
    height: f32
}

// combine two types
type FullUser = UserProfile & UserBody


fn printUser(user: FullUser?) { ... }
// equivalent to
fn printUser(user: FullUser | null )
```

Usually using `null` or `?` is a bad practice, but this is just for the sake of example.

Here is another example:

```java
let x: {name: String, age: u32} & {cool: bool} = {
    name: "praisethemoon",
    age: 30,
    cool: true
}

type RandomType = {name: String, age: u32} & {cool: bool}

fn printRandom(e: RandomType) { print(e.name) }
printRandom(x) // types are compatible.
```

Braces when used during expression usually means object creation.

## 9. Concurrency and Asynchronous behavior

type-c support has built-in support for concurrency and async behavior

### 9.1. Concurrency:

Spawning threads is done using `spawn` keyword, for example:

```rust
let threads = [1, 2, 3, 4];
for let t in threads {
  spawn {
    println("Thread {} is running", t);
  }
}
println("All threads spawned");
```

### 9.2. Asynchronous programming

Similar to typescript, type-c uses `async` to define async functions and `await` to trigger waiting for results.

For example:

```csharp
async fn downloadFile(url: string) -> string {
  // code to download the file
}

let contents = await downloadFile("https://example.com/file.txt");
```

## 10. Foreign Function Interface (FFI)

Example:

```rust
// Declare the OpenGL functions we want to use
extern "C" glcapi {
    fn glClear(mask: u32);
    fn glEnable(cap: u32);
    fn glClearColor(r: f32, g: f32, b: f32, a: f32);
    fn glBegin(mode: u32);
    fn glVertex2f(x: f32, y: f32);
    fn glEnd();
    fn glFlush();
}

// Define a function to draw a triangle using OpenGL
fn draw_triangle() {
    unsafe {
        glcapi.glClearColor(0.0, 0.0, 0.0, 0.0); // Set the clear color to black
        glcapi.glClear(0x4000); // Clear the color buffer

        glcapi.glEnable(0x0B71); // Enable vertex arrays

        glcapi.glBegin(0x0004); // Begin drawing triangles
        glcapi.glVertex2f(-0.5, -0.5);
        glcapi.glVertex2f(0.5, -0.5);
        glcapi.glVertex2f(0.0, 0.5);
        glcapi.glEnd(); // End drawing triangles

        glcapi.glFlush(); // Flush the OpenGL pipeline
    }
}

// Define the main function
fn main() {
    draw_triangle();
}
```