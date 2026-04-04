# @auth

Authentication helpers with session-backed login and registration flows.

## Install

```text
wevoa install auth
```

## Usage

```text
import "@auth"

let user = auth_register("demo@example.com", "secret123", { role: "admin" })
auth_login("demo@example.com", "secret123")
```
