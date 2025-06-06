# Contributing to GradBench

<!-- toc -->

- [Setup](#setup)
- [Dependencies](#dependencies)
- [CLI](#cli)
  - [Code formatters and linters](#code-formatters-and-linters)
- [Docker](#docker)
  - [Multi-platform images](#multi-platform-images)
- [Tools](#tools)
  - [Implementing a new eval for a tool](#implementing-a-new-eval-for-a-tool)
- [Evals](#evals)
- [JavaScript](#javascript)
  - [Website](#website)
- [Python](#python)
- [C++](#c)
- [Protocol](#protocol)
  - [Example](#example)
  - [Specification](#specification)
  - [Types](#types)

<!-- tocstop -->

GradBench welcomes contributions of every kind - simply issue a pull request
here on GitHub.

If you make a PR, you can add yourself to [CITATION.cff](/CITATION.cff) if you
are not already listed. This is however not mandatory if you would rather not be
listed as an author for some reason. Your name can always be added or removed
later if you change your mind.

Below you can find technical information on how GradBench is implemented, as
well as advice for making various kinds of contributions.

## Setup

First, clone this repo, e.g. with the [GitHub CLI][]:

```sh
gh repo clone gradbench/gradbench
```

Then open a terminal in your clone of it; for instance, if you cloned it via the
terminal, run this command:

```sh
cd gradbench
```

## Dependencies

You need [Docker][].

If you use [Nix][], pretty much everything else you need is in the `shell.nix`
file at the root of this repo.

Otherwise, make sure you have the following tools installed:

- [Python][]
- [Rust][]

These other tools are optional but useful:

- [Bun][]
- [uv][]
- [Make][]

## CLI

Many tasks make use of the GradBench CLI, which you can run via the
`./gradbench` script:

```sh
./gradbench --help
```

This script will always automatically build the CLI if it is not already up to
date.

### Code formatters and linters

This repository uses several linters and autoformatters. Use this command to run
them all:

```sh
./gradbench repo lint
```

If you are missing any, the error message should show instructions for how to
install what you are missing.

The code formatters, and some of the linters, can also automatically fix most
issues:

```sh
./gradbench repo lint --fix
```

If you use [VS Code][], our configuration in this repository should
automatically recommend that you install extensions for all the code formatters
we use, and run the relevant one anytime you save a file.

## Docker

Use the `run` subcommand to run a given eval on a given tool. You can use pass
any commands for the eval and tool, but to use the Docker images, the easiest
way is to use the `repo eval` and `repo tool` subcommands:

```sh
./gradbench run --eval "./gradbench repo eval hello" --tool "./gradbench repo tool pytorch"
```

Some evals support further configuration via their own CLI flags, which you can
see by passing `--help` to the eval itself:

```sh
./gradbench repo eval gmm -- --help
```

So for instance, to increase `n` for the GMM eval:

```sh
./gradbench run --eval "./gradbench repo eval gmm -- -n10000" --tool "./gradbench repo tool pytorch"
```

### Multi-platform images

The `repo eval` and `repo tool` subcommands are just for convenience when
building and running the Docker images locally; they do not build multi-platform
images. If you have followed the above instructions to configure Docker for
building such images, you can do so using the `--platform` flag on the
`repo build-eval` and `repo build-tool` subcommands:

```sh
./gradbench repo build-eval --platform linux/amd64,linux/arm64 hello
./gradbench repo build-tool --platform linux/amd64,linux/arm64 pytorch
```

This typically takes much longer, so it tends not to be convenient for local
development. However, if a tool does not support your machine's native
architecture, emulation may be your only option, in which case you can select
just one platform which is supported by that tool:

```sh
./gradbench run --eval "./gradbench repo eval hello" --tool "./gradbench repo tool --platform linux/amd64 scilean"
```

## Tools

If you'd like to contribute a new tool: awesome! We're always excited to expand
the set of automatic differentiation tools in GradBench. The main thing you need
to do is create a subdirectory under the `tools` directory in this repo, and
create a `Dockerfile` in that new subdirectory. Other than having an
`ENTRYPOINT`, you can pretty much do whatever you want; take a look at the
already-supported tools to see some examples! You must include the following as
the last line in your `Dockerfile`, though:

```Dockerfile
LABEL org.opencontainers.image.source=https://github.com/gradbench/gradbench
```

We'd also really appreciate it if you also write a short `README.md` file next
to your `Dockerfile`; this can be as minimal as just a link to the tool's
website, but can also include more information, e.g. anything specific about
this setup of that tool for GradBench.

Before taking a look at any of the other evals, you should implement the
[`hello` eval](evals/hello) for the tool you're adding! This will help you get
all the structure for the GradBench protocol working correctly first, after
which you can implement other evals for that tool over time. Once you've done
so, add a file called `evals.txt` in your tool directory (next to your
`Dockerfile`) with the names of all the evals your tool supports, each on their
own line, in sorted order; otherwise GitHub Actions will squawk at you saying it
expected your tool to be `undefined` on those evals.

If the new tool you want to add is a C++ or Python library, then you are in
luck - you can piggyback on the existing implementations of the procotol.
Otherwise, you will have to implement it yourself. If you have access to a JSON
library in your chosen language, this is not so difficult. Using `gradbench`
with the `-o` option, to make it dump the raw message log to a file, is a good
way to debug errors in the protocol implementation. Even if your program is not
written in Python, you may still find it beneficial to use the Python
implementation of the protocol, and then internally execute your program(s)
using some bespoke mechanism. That is in fact
[how the C++ tools work](python/gradbench/gradbench/cpp.py).

### Implementing a new eval for a tool

For some tools, the infrastructure has been built (speaking the protocol,
writing the `Dockerfile`), but not yet implementations of all benchmarks.
Sometimes this is because we have not gotten around to it, but at other times it
is because those benchmarks require something that is tricky to do in a specific
tool.

If you are looking for a missing implementation to add, try browsing the
[missing](https://github.com/gradbench/gradbench/labels/missing) issue label.

To add a new implementation, the easiest approach is to pattern match based on
an existing implementations. For the C++ tools, you need to add a program
`foo.cpp` where `foo` is the name of the benchmark. I suggest looking at the
[Enzyme](evals/enzyme) implementations for the boilerplate input/output reading
code, as all benchmarks have been implemented in Enzyme.

## Evals

Adding an eval is the most laborious form of contribution. An eval must be
specified in a way that is clear enough for others to understand it, come with
some validation mechanism, and also have at least a couple of implementations
using various tools.

Similarly to tools, an eval is specified by a subdirectory in
[evals/](https://github.com/gradbench/gradbench/tree/main/evals) that behaves
like an eval process as specified in the protocol. There is no real limit to
what an eval can do, except that it must begin by sending a `start` message,
then in most cases follow with a `define` message and some `evaluate` messages
with various functions and inputs. All of the GradBench evals are currently
written in Python - this is not a hard requirement, but since evals are not
performance-sensitive or particularly complicated, writing them in Python means
you can reuse existing utility libraries.

Beyond the technical effort of specifying and implementing a benchmark, another
question is which benchmarks are _worthwhile_. The whole point of GradBench is
comparison, so a benchmark is only worth having if there is an expectation that
it shows something interesting related to AD, and _will be implemented by
multiple tools_. If you are in doubt, come and talk to us.

As a special case, GradBench is almost always willing to accept an eval that
matches a benchmark found in an existing AD benchmark suite, as GradBench aims
to (benevolently!) assimilate all current benchmark suites.

## JavaScript

We use Bun for JavaScript code in this repository. First install all
dependencies from npm:

```sh
bun install
```

### Website

We use [Vite][] for the website. To develop the website locally, run this
command:

```sh
bun run --filter=@gradbench/website dev
```

This will log a `localhost` URL to your terminal; open that URL in your browser.
Any changes you make to files in `js/website/src` should automatically appear.

## Python

The Docker images should be considered canonical, but for local development, it
can be more convenient to instead install and run tools directly. You can use
`uv run` to do this:

```sh
./gradbench run --eval "./gradbench repo eval hello" --tool "uv run python/gradbench/gradbench/tools/pytorch/run.py"
```

## C++

Some tools make use of C++ code shared in the `cpp` directory; if doing local
development with any of those tools, you must first run the following command:

```sh
make -C cpp
```

## Protocol

GradBench decouples benchmarks from tools via a [JSON][]-based protocol. In this
protocol, there is an _intermediary_ (the `run` subcommand of our `gradbench`
CLI), an _eval_, and a _tool_. The eval and the tool communicate with each other
by sending and receiving messages over stdout and stdin, which are intercepted
and forwarded by the intermediary.

### Example

To illustrate, here is a hypothetical example of a complete session of the
protocol, as captured and reported by the intermediary:

```jsonl
{ "elapsed": { "nanoseconds": 100000 }, "message": { "id": 0, "kind": "start" } }
{ "elapsed": { "nanoseconds": 150000 }, "response": { "id": 0 } }
{ "elapsed": { "nanoseconds": 200000 }, "message": { "id": 1, "kind": "define", "module": "foo" } }
{ "elapsed": { "nanoseconds": 250000 }, "response": { "id": 1, "success": true } }
{ "elapsed": { "nanoseconds": 300000 }, "message": { "id": 2, "kind": "evaluate", "module": "foo", "function": "bar", "input": 3.14159 } }
{ "elapsed": { "nanoseconds": 350000 }, "response": { "id": 2, "success": true, "output": 2.71828, "timings": [{ "name": "evaluate", "nanoseconds": 5000000 }] } }
{ "elapsed": { "nanoseconds": 400000 }, "message": { "id": 3, "kind": "analysis", "of": 2, "valid": false, "error": "Expected tau, got e." } }
{ "elapsed": { "nanoseconds": 450000 }, "response": { "id": 3 } }
{ "elapsed": { "nanoseconds": 500000 }, "message": { "id": 4, "kind": "evaluate", "module": "foo", "function": "baz", "input": { "mynumber": 121 } } }
{ "elapsed": { "nanoseconds": 550000 }, "response": { "id": 4, "success": true, "output": { "yournumber": 342 }, "timings": [{ "name": "evaluate", "nanoseconds": 7000000 }] } }
{ "elapsed": { "nanoseconds": 600000 }, "message": { "id": 5, "kind": "analysis", "of": 4, "valid": true } }
{ "elapsed": { "nanoseconds": 650000 }, "response": { "id": 5 } }
```

Here is that example from the perspectives of the eval and the tool.

- Output from the eval, or equivalently, input to the tool:
  ```jsonl
  { "id": 0, "kind": "start" }
  { "id": 1, "kind": "define", "module": "foo" }
  { "id": 2, "kind": "evaluate", "module": "foo", "function": "bar", "input": 3.14159 }
  { "id": 3, "kind": "analysis", "of": 2, "valid": false, "error": "Expected tau, got e." }
  { "id": 4, "kind": "evaluate", "module": "foo", "function": "baz", "input": { "mynumber": 121 } }
  { "id": 5, "kind": "analysis", "of": 4, "valid": true }
  ```
- Output from the tool, or equivalently, input to the eval:
  ```jsonl
  { "id": 0 }
  { "id": 1, "success": true }
  { "id": 2, "success": true, "output": 2.71828, "timings": [{ "name": "evaluate", "nanoseconds": 5000000 }] }
  { "id": 3 }
  { "id": 4, "success": true, "output": { "yournumber": 342 }, "timings": [{ "name": "evaluate", "nanoseconds": 7000000 }] }
  { "id": 5 }
  ```

As shown by this example, the intermediary forwards every message from the eval
to the tool, and vice versa. The tool only provides substantive responses for
`"define"` and `"evaluate"` messages; for all others, it simply gives a response
acknowledging the `"id"` of the original message.

### Specification

The session proceeds over a series of _rounds_, driven by the eval. In each
round, the eval sends a _message_ with a unique `"id"`, and the tool sends a
_response_ with that same `"id"`. The message also includes a `"kind"`, which
has four possibilities:

1. `"kind": "start"` - the eval always sends this message first, waiting for the
   tool's response to ensure that it is ready to receive further messages. This
   message may optionally contain the `"eval"` name, and the response may
   optionally contain the `"tool"` name and/or a `"config"` field that contains
   arbitrary information about how the tool or eval has been configured. This
   information can be used by programs that do offline processing of log files,
   but is not otherwise significant to the protocol.

2. `"kind": "define"` - the eval provides the name of a `"module"` which the
   tool will need in order to proceed further with this particular benchmark.
   This will allow the tool to respond saying whether or not it knows of and has
   an implementation for the module of that name.

   - The tool responds with the `"id"` and either `"success": true` or
     `"success": false`. In the former case, the benchmark proceeds normally. In
     the latter case, the tool is indicating that it does not have an
     implementation for the requested module, and the eval should stop and not
     send any further messages; the tool may also optionally include an
     `"error"` string. In either case, the tool may optionally provide a list of
     `"timings"` for subtasks of preparing the requested module.

3. `"kind": "evaluate"` - the eval again provides a `"module"` name, as well as
   the name of a `"function"` in that module. Currently there is no formal
   process for registering module names or specifying the functions available in
   those modules; those are specified informally via documentation in the evals
   themselves. An `"input"` to that function is also provided; the tool will be
   expected to evaluate that function at that input, and return the result.
   Optionally, the eval may also provide a short human-readable `"description"`
   of the input. The precise form of the `"input"` depends on the eval in
   question. However, many evals require `"input"` to be an object with (among
   others) the fields `"min_runs"` and `"min_seconds`". The tool must then
   evaluate the function a minimum of `"min_runs"` times or until the
   accumulated runtime exceeds `"min_seconds"`, whichever is longer. The runtime
   measurements of each function evaluation must be returned as a separate
   timing, as described below.

   - The tool responds with the `"id"` and whether or not it had `"success"`
     evaluating the function on the given input. If `"success": true` then the
     response must also include the resulting `"output"`; otherwise, the
     response may optionally include an `"error"` string. Optionally, the tool
     may also provide a list of `"timings"` for subtasks of the computation it
     performed. Each timing must include a `"name"` that does not need to be
     unique, and a number of `"nanoseconds"`. Currently, most tools only provide
     one entry in `"timings"`: an `"evaluate"` entry, which by convention means
     the amount of time that tool spent evaluating the function itself, not
     including other time such as JSON encoding/decoding.

4. `"kind": "analysis"` - the eval provides the ID of a prior `"evaluate"`
   message it performed analysis `"of"`, along with a boolean saying whether the
   tool's output was `"valid"`. If the output was invalid, the eval can also
   provide an `"error"` string explaining why.

If the tool receives any message whose `"kind"` is neither `"define"` nor
`"evaluate"`, it must always respond, but does not need to include anything
other than the `"id"`.

### Types

Here is a somewhat more formal description of the protocol using [TypeScript][]
types. Some of the types are not used directly, or in all evals, but may be
referenced by eval-specific protocol descriptions via an `import`
`from "gradbench"`. In particular, the value expected in the `"input"` field of
an `"EvaluateMessage"` is specific to each eval.

```typescript
// These are types in the core protocol, described above.

export type Id = number;

export interface Base {
  id: Id;
}

export interface Duration {
  nanoseconds: number;
}

export interface Timing extends Duration {
  name: string;
}

export interface StartMessage extends Base {
  kind: "start";
  eval?: string;
  config?: any;
}

export interface DefineMessage extends Base {
  kind: "define";
  module: string;
}

export interface EvaluateMessage extends Base {
  kind: "evaluate";
  module: string;
  function: string;
  input: any;
  description?: string;
}

export interface AnalysisMessage extends Base {
  kind: "analysis";
  of: Id;
  valid: boolean;
  error?: string;
}

export type Message =
  | StartMessage
  | DefineMessage
  | EvaluateMessage
  | AnalysisMessage;

export interface StartResponse extends Base {
  tool?: string;
  config?: any;
}

export interface DefineResponse extends Base {
  success: boolean;
  timings?: Timing[];
  error?: string;
}

export interface EvaluateResponse extends Base {
  success: boolean;
  output?: any;
  timings?: Timing[];
  error?: string;
}

export type Response = Base | StartResponse | DefineResponse | EvaluateResponse;

export interface Line {
  elapsed: Duration;
}

export interface MessageLine extends Line {
  message: Message;
}

export interface ResponseLine extends Line {
  response: Response;
}

export type Session = (MessageLine | ResponseLine)[];

// These are auxiliary types used by some evals.

/** An integer. */
export type Int = number;

/** A double-precision floating point value. */
export type Float = number;

/**
 * Fields to be included in the input of an eval requesting a tool to run a
 * function multiple times in a single evaluate message. The tool's response
 * should include one timing entry with the name `"evaluate"` for each time it
 * ran the function.
 */
export interface Runs {
  /** Evaluate the function at least this many times. */
  min_runs: number;

  /** Evaluate the function until the total time exceeds this many seconds. */
  min_seconds: number;
}
```

[bun]: https://bun.sh/
[containerd]: https://docs.docker.com/storage/containerd/
[docker]: https://docs.docker.com/engine/install/
[github cli]: https://github.com/cli/cli#installation
[json]: https://json.org/
[make]: https://en.wikipedia.org/wiki/Make_(software)
[multi-platform images]: https://docs.docker.com/build/building/multi-platform/
[nix]: https://nixos.org/
[python]: https://docs.astral.sh/uv/guides/install-python/
[qemu]:
  https://docs.docker.com/build/building/multi-platform/#install-qemu-manually
[rust]: https://www.rust-lang.org/tools/install
[typescript]: https://www.typescriptlang.org/
[uv]: https://docs.astral.sh/uv
[vite]: https://vitejs.dev/
[vs code]: https://code.visualstudio.com/
