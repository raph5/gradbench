import argparse
import json
import sys
import traceback
from importlib import import_module

from gradbench.wrap import Wrapped


def resolve(module, name):
    functions = import_module(module)
    return getattr(functions, name)


def run(params):
    func: Wrapped = resolve(params["module"], params["function"])
    return func.wrapped(params["input"])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--multithreaded", action="store_true")
    args = parser.parse_args()

    if args.multithreaded:
        print("multithreading not yet implemented", file=sys.stderr)

    for line in sys.stdin:
        message = json.loads(line)
        response = {}
        if message["kind"] == "start":
            response["tool"] = "tensorflow"
        elif message["kind"] == "evaluate":
            response = run(message)
        elif message["kind"] == "define":
            try:
                import_module(message["module"])
                response["success"] = True
            except Exception as e:
                response["error"] = "".join(traceback.format_exception(e))
                response["success"] = False
        print(json.dumps({"id": message["id"]} | response), flush=True)


if __name__ == "__main__":
    main()
