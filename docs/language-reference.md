# Language Reference

This document describes the currently supported WevoaWeb language surface.

## Variables

Mutable variables:

```text
let count = 10
```

Constants:

```text
const title = "WevoaWeb"
```

## Data Types

Current runtime value types:

- `integer`
- `string`
- `boolean`
- `nil`
- callable values

## Expressions

Supported operators:

- `+`
- `-`
- `*`
- `/`
- `>`
- `<`
- `>=`
- `<=`
- `==`
- `!=`
- unary `-`
- unary `!`

String concatenation works through `+` when either operand is a string.

## Blocks

```text
{
let name = "inner"
print(name)
}
```

Blocks create a new lexical scope.

## Conditionals

```text
if (count > 5) {
print("big")
} else {
print("small")
}
```

## Loops

```text
loop (i = 0; i < 3; i = i + 1) {
print(i)
}
```

## Functions

```text
func greet(name) {
return "Hello " + name
}
```

Functions support:

- parameters
- return values
- lexical closures

## Built-in Functions

### `print()`

Prints its arguments separated by spaces and ends with a newline.

```text
print("Hello", 10)
```

### `input()`

Reads a line from standard input.

```text
let name = input("Name: ")
```

## Routes

Routes are a language-level feature for the web runtime.

```text
route "/" {
return "<h1>Home</h1>"
}
```

Rules:

- route paths must evaluate to strings
- route paths must start with `/`
- duplicate routes are rejected

## Comments

Single-line comments:

```text
// this is a comment
```

## Source Diagnostics

The lexer, parser, and runtime attach source spans to errors using line and column information.

## Not Yet Supported

- arrays
- objects or maps
- modules and imports
- classes
- floating-point numbers
- `break` and `continue`
- logical `&&` and `||`
