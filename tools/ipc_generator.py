#!/usr/bin/env python3

import os
import sys
from dataclasses import dataclass
from datetime import datetime
from typing import List

from typeguard import install_import_hook

install_import_hook("__main__")


@dataclass
class Arg:
    name: str
    type: str | None = None
    indirect: bool = False
    input: bool = False
    output: bool = False
    wide: bool = False
    object: bool = False

    def __repr__(self) -> str:
        s = ""

        if self.input:
            if self.output:
                if self.wide:
                    s += "inout64 "
                else:
                    s += "inout "
            else:
                if self.wide:
                    s += "in64 "
                else:
                    s += "in "
        else:
            if self.output:
                if self.wide:
                    s += "out64 "
                else:
                    s += "out "
            else:
                if self.wide:
                    s += "invalid64 "
                else:
                    s += "invalid "

        if self.object:
            s += "obj "

        s += f"{self.name}: "
        if self.indirect:
            s += "*"
        if self.type is None:
            s += "data"
        else:
            s += self.type

        return s


@dataclass
class Method:
    name: str
    retval: str
    args: List[Arg]


@dataclass
class ObjType:
    name: str
    methods: List[Method]


def parse_block(block_content: str) -> dict[str, List[List[str]]]:
    """Parse block content into methods by splitting on semicolons."""
    # Split by semicolon
    statements = [s.strip() for s in block_content.split(";") if s.strip()]

    methods: dict[str, List[List[str]]] = {}
    for stmt in statements:
        # Split by comma to get fields
        fields = [f.strip() for f in stmt.split(",")]

        # Split each field by whitespace
        parsed_fields: List[List[str]] = []
        for field in fields:
            parts = field.split()
            parsed_fields.append(parts)

        method_name = parsed_fields[0][0]

        methods[method_name] = parsed_fields

    return methods


def parse_ipc_file(content: str) -> dict[str, dict[str, dict[str, List[List[str]]]]]:
    """Parse IPC definition file into structured blocks."""
    # Remove all newlines and extra whitespace - treat as continuous stream
    content = " ".join(content.split())

    i = 0

    data: dict[str, dict[str, dict[str, List[List[str]]]]] = {}

    while i < len(content):
        # Find block start
        brace_pos = content.find("{", i)
        if brace_pos == -1:
            break

        # Extract block header (everything before {)
        header = content[i:brace_pos].strip().split()
        block_type = header[0]
        block_name = header[1]

        if block_type not in data:
            data[block_type] = {}

        # Find closing brace (no nesting)
        close_brace = content.find("}", brace_pos)
        if close_brace == -1:
            break

        # Extract block content (between braces)
        block_content = content[brace_pos + 1 : close_brace].strip()
        data[block_type][block_name] = parse_block(block_content)

        i = close_brace + 1

    return data


def parse_objs(data: dict[str, dict[str, dict[str, List[List[str]]]]]) -> List[ObjType]:
    objs: List[ObjType] = []

    for obj_name in data["obj"]:
        obj = ObjType(obj_name, [])
        objs.append(obj)

        for method_name in data["obj"][obj_name]:
            retval = data["obj"][obj_name][method_name][1][0]
            method = Method(method_name, retval, [])
            obj.methods.append(method)

            for argdata in data["obj"][obj_name][method_name][2:]:
                arg = Arg(argdata.pop())
                method.args.append(arg)

                arg.indirect = arg.name.startswith("*")
                if arg.indirect:
                    arg.name = arg.name[1:]

                if len(argdata) > 0:
                    fst = argdata[0]

                    arg.input = (
                        fst == "in"
                        or fst == "in64"
                        or fst == "inout"
                        or fst == "inout64"
                    )
                    arg.output = (
                        fst == "out"
                        or fst == "out64"
                        or fst == "inout"
                        or fst == "inout64"
                    )
                    arg.wide = fst == "in64" or fst == "out64" or fst == "inout64"
                    if arg.input or arg.output:
                        argdata = argdata[1:]
                else:
                    arg.input = True
                    arg.output = False
                    arg.wide = False

                if not arg.output:
                    arg.input = True

                arg.object = len(argdata) > 0 and argdata[0] == "obj"
                if arg.object:
                    argdata = argdata[1:]

                if len(argdata) > 0:
                    arg.type = argdata[0]
                else:
                    arg.type = None

    return objs


# Main program
if len(sys.argv) != 4:
    print(f"Usage: {sys.argv[0]} <input_file> <c_output> <h_output>", file=sys.stderr)
    sys.exit(1)

with open(sys.argv[1], "r") as f:
    content = f.read()

header_name = sys.argv[3]

c_out = open(sys.argv[2], "w")
h_out = open(header_name, "w")

data = parse_ipc_file(content)

objs = parse_objs(data)

indent = 0


def print_c(*values: object, sep: str | None = " ", end: str | None = "\n"):
    for x in range(indent):
        print("\t", end="", file=c_out)

    print(*values, file=c_out, sep=sep, end=end)


def print_h(*values: object, sep: str | None = " ", end: str | None = "\n"):
    print(*values, file=h_out, sep=sep, end=end)


def arg_signature(a: Arg) -> List[str]:
    if a.output:
        if a.object:
            return [f"{a.type}_t **{a.name}"]
        elif a.type == "str":
            return [
                f"char *{a.name}",
                f"size_t {a.name}_size",
            ]
        elif a.type == "str" or a.type is None:
            return [
                f"ipc_buffer_t *{a.name}",
                f"size_t {a.name}_slice",
            ]
        else:
            return [f"{a.type} *{a.name}"]
    else:
        if a.object:
            return [f"{a.type}_t *{a.name}"]
        elif a.type == "str":
            return [f"const char *{a.name}"]
        elif a.type is None:
            return [
                f"const ipc_blob_t *{a.name}",
                f"size_t {a.name}_slice",
            ]
        elif a.indirect:
            return [f"const {a.type} *{a.name}"]
        else:
            return [f"{a.type} {a.name}"]


def method_signature(method: Method) -> List[str]:
    argdecl: List[str] = []
    for a in method.args:
        argdecl += arg_signature(a)

    return argdecl


def print_both(*values: object):
    print(*values, file=c_out)
    print(*values, file=h_out)


def indata_structdecl(args: List[Arg], merge_inputs: bool):
    print_c("struct __attribute__((packed)) {")
    for a in args:
        if a.type == "str" or a.type is None:
            if merge_inputs:
                print_c(f"\tsize_t {a.name}_slice;")
        elif a.input and (a.indirect or merge_inputs) and not a.object:
            print_c(f"\t{a.type} {a.name};")
    print_c("} _indata;")
    print_c()


print_both("// Autogenerated IPC server implementation.")
print_both("// Command: ", *sys.argv)
print_both(
    "// Source timestamp: ", datetime.fromtimestamp(os.path.getmtime(sys.argv[1]))
)
print_both()

print_h("#include <stddef.h>")
print_h("#include <stdlib.h>")
print_h("#include <ipc_b.h>")
print_c(f'#include "{os.path.basename(header_name)}"')
print_both()

for obj in objs:
    if not obj.methods:
        continue

    server = obj.name
    methods = obj.methods

    if True:
        print_h(f"typedef struct {server}_impl {server}_impl_t;")
        print_h(f"typedef struct {server} {server}_t;")
        print_h(f"typedef struct {server}_ops {server}_ops_t;")
        print_h()

        print_h(f"struct {server}_ops {{")
        print_h("\tsize_t _sizeof;")
        print_h(
            f"\tvoid (*_handle_message)({server}_impl_t *self, const ipc_message_t *msg);"
        )
        for method in methods:
            print_h(
                f"\t{method.retval} (*{method.name})({', '.join([f'{server}_impl_t *self'] + method_signature(method))});",
            )

        print_h("};")
        print_h()
        print_h(
            f"void {server}_handle_message({server}_impl_t *self, const ipc_message_t *msg);"
        )
        print_h()
        for method in methods:
            print_h(
                f"{method.retval} {server}_{method.name}({', '.join([f'{server}_t *self'] + method_signature(method))});",
            )

    print_c(f"enum {server}_methods {{")
    indent += 1
    print_c(f"_{server}_op_undef,")
    for method in methods:
        print_c(f"_{server}_op_{method.name},")
    indent -= 1
    print_c("};")
    print_c()

    print_c(
        f"void {server}_handle_message({server}_impl_t *self, const ipc_message_t *msg)"
    )
    print_c("{")
    indent += 1

    print_c(f"{server}_ops_t *ops = *({server}_ops_t **) self;")
    print_c()

    print_c("switch (ipcb_get_val_1(msg)) {")

    for method in methods:
        # 0: return endpoint
        # 1: method number
        # four spare arguments for method args
        #
        # if we can put all arguments in directly, we do so, otherwise we go
        # with single ipc_blob_t for all args

        slots_needed_in = 0
        indirect_slot_in = 0
        slots_needed_out = 0
        indirect_slot_out = 0

        for a in method.args:
            if a.type == "str" or a.type is None:
                slots_needed_in += 2
                continue

            if a.input:
                if a.indirect and not a.object:
                    indirect_slot_in = 1
                elif a.wide:
                    slots_needed_in += 2
                else:
                    slots_needed_in += 1

            if a.output:
                if a.indirect and not a.object:
                    indirect_slot_out = 1
                elif a.wide:
                    slots_needed_out += 2
                else:
                    slots_needed_out += 1

        merge_inputs = (slots_needed_in + indirect_slot_in) > 4
        merge_outputs = (slots_needed_out + indirect_slot_out) > 4

        print_c()
        print_c(f"/* {method} */")
        print_c()

        print_c(f"case _{server}_op_{method.name}:")
        indent += 1
        print_c("{")
        indent += 1

        print_c("// TODO: check message type and detect protocol mismatch")

        # passing sizeof(ops) from the caller allows safely adding entries while
        # maintaining compatibility with code linked to older/newer version
        print_c(
            f"if (offsetof(typeof(*ops), {method.name}) + sizeof(ops->{method.name}) > ops->_sizeof || !ops->{method.name}) {{"
        )
        indent += 1
        print_c("ipcb_answer_protocol_error(msg);")
        print_c("return;")
        indent -= 1
        print_c("}")
        print_c()

        arg_idx = 2

        # TODO: named declaration for indata type

        if merge_inputs or indirect_slot_in > 0:
            indata_structdecl(method.args, merge_inputs)
            print_c(f"ipc_blob_read_{arg_idx}(&msg, &_indata, sizeof(_indata));")
            print_c()
            arg_idx += 1

        if merge_inputs:
            indata_prefix = "_indata."
        else:
            indata_prefix = ""

        allocs = []

        for a in method.args:
            if a.type == "str" or a.type is None:
                if not merge_inputs:
                    print_c(f"size_t {a.name}_slice = ipcb_get_val_{arg_idx}(&msg);")
                    arg_idx += 1

                print_c(
                    f"size_t {a.name}_len = ipcb_slice_len({indata_prefix}{a.name}_slice);"
                )
                print_c(f"void *{a.name} = calloc({a.name}_len, 1);")
                print_c(f"if ({a.name} == nullptr) {{")
                indent += 1
                print_c("ipcb_answer_nomem(msg);")
                for alloc in allocs:
                    print_c(f"free({alloc});")
                print_c("return;")
                indent -= 1
                print_c("}")
                allocs += [a.name]

                print_c()

                if a.input:
                    print_c(f"ipc_blob_read_{arg_idx}(&msg, {a.name}, {a.name}_slice);")
                    if a.type == "str":
                        print_c(f"{a.name}[{a.name}_len - 1] = '\\0';")
                else:
                    print_c(f"ipcb_buffer_t {a.name}_obj = ipc_get_obj_{arg_idx}(msg);")

                print_c()
                arg_idx += 1
            elif a.input:
                if a.object:
                    print_c(f"{a.type} {a.name} = ipcb_get_obj{arg_idx}(&msg);")
                    arg_idx += 1
                elif not a.indirect and a.wide and not merge_inputs:
                    print_c(f"{a.type} {a.name} = ipcb_get_val64_{arg_idx}(&msg);")
                    arg_idx += 2
                elif not a.indirect and not merge_inputs:
                    print_c(f"{a.type} {a.name} = ipcb_get_val{arg_idx}(&msg);")
                    arg_idx += 1

        for a in method.args:
            if a.output and a.type != "str" and a.type is not None:
                print_c(f"{a.type} {a.name};")
            if a.input and a.indirect and a.type is not None and a.type != "str":
                print_c(f"{a.type} {a.name} = _indata.{a.name};")

        print_c(f"{method.retval} rc = ops->{method.name}(", end="")

        arglist = ["self"]

        for a in method.args:
            if a.indirect and a.type is None:
                arglist += [a.name, f"{a.name}_len"]
            elif a.indirect and a.type == "str":
                arglist += [a.name]
            elif a.output or a.indirect:
                arglist += ["&" + a.name]
            else:
                arglist += [indata_prefix + a.name]

        print_c(*arglist, sep=", ", end=");\n")

        for a in method.args:
            if a.output and a.indirect and (a.type is None or a.type == "str"):
                print_c(
                    f"ipcb_buffer_write({a.name}_obj, {indata_prefix + a.name}_slice, {a.name}, {a.name}_len);"
                )

        print_c("ipcb_message_t answer = ipcb_start_answer(&msg, rc);")

        arg_idx = 1

        if merge_outputs or indirect_slot_out > 0:
            print_c()
            print_c("struct __attribute__((packed)) {")
            indent += 1
            for a in method.args:
                if a.output:
                    if a.type == "str":
                        print_c(f"size_t {a.name}_len;")
                    elif (a.indirect and a.type is not None) or (
                        not a.indirect and merge_inputs
                    ):
                        print_c(f"{a.type} {a.name};")
            indent -= 1
            print_c("} _outdata = {")
            indent += 1
            for a in method.args:
                if a.output:
                    if a.type == "str":
                        print_c(f".{a.name}_len = {a.name}_len,")
                    elif (a.indirect and a.type is not None) or (
                        not a.indirect and merge_inputs
                    ):
                        print_c(f".{a.name} = {a.name},")
            indent -= 1
            print_c("};")

            print_c(f"ipc_blob_write_{arg_idx}(&answer, &_outdata, sizeof(_outdata));")
            arg_idx += 1
            print_c()

        if not merge_outputs:
            for a in method.args:
                if a.output:
                    if not a.indirect:
                        print_c(f"ipcb_set_val_{arg_idx}(&answer, {a.name});")
                        arg_idx += 1
                    elif a.type == "str":
                        print_c(f"ipcb_set_val_{arg_idx}(&answer, {a.name}_len);")
                        arg_idx += 1

        print_c("ipcb_send_answer(&msg, answer);")

        for alloc in allocs:
            print_c(f"free({alloc});")

        print_c("return;")
        indent -= 1
        print_c("}")
        indent -= 1

    print_c("default:")
    indent += 1
    print_c("ipcb_answer_protocol_error(msg);")
    indent -= 1
    print_c("}")
    indent -= 1
    print_c("}")

    for method in methods:
        print_c()
        print_c(
            f"{method.retval} {server}_{method.name}({', '.join([f'{server}_t *self'] + method_signature(method))})",
        )
        print_c("{")
        indent += 1

        indent -= 1
        print_c("}")
