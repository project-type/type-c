type-c
===
`type-c`: Compiler and `type-v`: VM.

## About:
`type-c` is a strong statically typed programming language. 
The compiler is at 10% progress and features  might change.

Planned Features:
- Static typing & Strong type inference engine
- Anonymous functions and types. Type checking in based on structure rather than layout. 
- Interface oriented programming (classes cannot extend classes, only implement behaviors)
- Functional programming
- Mutability by default (yes it's a feature)
- Generics
- Built-in coroutines and async/await supports
- Solid `ffi`, data structures designed to be easily used with C libraries

## Hello world
```
fn main() =
    print("Hello world!")
```

## Want more?
```
type User = struct {
    name: string,
    profession: string,
    age: u8,
}

fn main() {
    let user1 = {name: "John", age: u8(42)} // type casting is similar to function call.
    let user2: User = {"Jane", "Twitch streamer", 24} // 
    
    let {name: string} = user1 // destructuring, type is optional
}
```

## FAQ:
- What is `type-c`? A programming language.
- Why? Because I can.
- What sets `type-c` apart? For now it doesn't exist. The goal is to design a language that is highly expressive, easy to use, and fast.
- What is `type-v`? The VM that will run type-c.
- Can I contribute? Ideas? Yes. Code? Probably not.
- When will the project be finished? Never. It will always be a work in progress.

## TODO:
- [ ] Think about decorators, especially to preserve struct layout, useful for ffi
- [ ] Think about mutability
- [ ] Think about partial function applications
- [ ] Think about async/await and coroutines
- [ ] Improve error handling
- [ ] Improve memory management

## Roadmap:
1. `type-c`: A compiler for the `type-c` language, with full type checking and solid optimizations.
2. `type-v`: The virtual machine for `type-c`, a register based VM able to fully run `type-c` programs.
3. Improve `ffi`
4. Standard Library
5. VSCode plugin
6. Improving `type-v`: JIT, GC, etc.
7. More `type-v` optimizations.
8. Game engine
9. OS
10. Big Brain AI

## How to contribute
Well, not much you can do now in terms of code, but you can try lifting my spirit by giving me a star and maybe sharing your thoughts.
I would rather keep working on this project alone for now as its a learning experience, but if you want to help me
with solid ideas and foundations, please go ahead.

I very much welcome anyone to challenge my design and detect any flaws.

Reach out:
- Tweet me [@alonnesora](https://twitter.com/alonnesora) 
- Or email: doit@praisethemoon.org

## Third party libraries:

| Library | Usage | Source | Link | License |
| :---: | :---: | :---: | :---: |:---:|
| `vec` | Dynamic array | `src/vec.h` | [Link](https://github.com/rxi/vec)| MIT |
| `map` | Hash map | `src/map.h` | [Link](https://github.com/rxi/map) | MIT |
| `minunit` | Unit testing | `src/minunit.h` | [Link](https://github.com/siu/minunit) | MIT |


