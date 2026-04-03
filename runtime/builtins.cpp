#include "runtime/builtins.h"

#include "interpreter/interpreter.h"
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
}

}  // namespace wevoaweb
