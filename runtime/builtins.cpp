#include "runtime/builtins.h"

#include "interpreter/interpreter.h"
#include "runtime/sqlite_module.h"
#include "utils/error.h"

namespace wevoaweb {

void registerBuiltins(Interpreter& interpreter) {
    interpreter.registerNative(
        "print",
        std::nullopt,
        [](Interpreter& runtime, const std::vector<Value>& arguments, const SourceSpan&) -> Value {
            for (std::size_t i = 0; i < arguments.size(); ++i) {
                if (i > 0) {
                    runtime.output() << ' ';
                }
                runtime.output() << arguments[i].toString();
            }
            runtime.output() << '\n';
            return Value {};
        });

    interpreter.registerNative(
        "input",
        std::nullopt,
        [](Interpreter& runtime, const std::vector<Value>& arguments, const SourceSpan& span) -> Value {
            if (arguments.size() > 1) {
                throw RuntimeError("input() accepts zero or one argument.", span);
            }

            if (!arguments.empty()) {
                runtime.output() << arguments.front().toString();
                runtime.output().flush();
            }

            std::string line;
            std::getline(runtime.input(), line);
            return Value(std::move(line));
        });

    interpreter.registerNative(
        "view",
        std::nullopt,
        [](Interpreter& runtime, const std::vector<Value>& arguments, const SourceSpan& span) -> Value {
            if (arguments.empty() || arguments.size() > 2) {
                throw RuntimeError("view() accepts one or two arguments.", span);
            }

            if (!arguments[0].isString()) {
                throw RuntimeError("view() template path must be a string.", span);
            }

            const Value context = arguments.size() == 2 ? arguments[1] : Value(Value::Object {});
            return Value(runtime.renderView(arguments[0].asString(), context, span));
        });

    interpreter.registerNative(
        "len",
        1,
        [](Interpreter&, const std::vector<Value>& arguments, const SourceSpan& span) -> Value {
            const Value& value = arguments.front();

            if (value.isString()) {
                return Value(static_cast<std::int64_t>(value.asString().size()));
            }
            if (value.isArray()) {
                return Value(static_cast<std::int64_t>(value.asArray().size()));
            }
            if (value.isObject()) {
                return Value(static_cast<std::int64_t>(value.asObject().size()));
            }

            throw RuntimeError("len() expects a string, array, or object.", span);
        });

    interpreter.registerNative(
        "append",
        2,
        [](Interpreter&, const std::vector<Value>& arguments, const SourceSpan& span) -> Value {
            if (!arguments[0].isArray()) {
                throw RuntimeError("append() expects an array as its first argument.", span);
            }

            Value::Array result = arguments[0].asArray();
            result.push_back(arguments[1]);
            return Value(std::move(result));
        });

    registerSqliteModule(interpreter);
}

}  // namespace wevoaweb
