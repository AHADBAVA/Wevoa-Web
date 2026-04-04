# @db

Convenience helpers around the configured SQLite connection.

## Install

```text
wevoa install db
```

## Usage

```text
import "@db"

let total = db_scalar("SELECT COUNT(*) FROM users")
```
