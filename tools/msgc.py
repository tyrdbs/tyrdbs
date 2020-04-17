#!/usr/bin/env python

import sys
import re
import json
import jinja2
import argparse

from collections import namedtuple
from collections import OrderedDict


native_types = {
    'int8': 'int8_t',
    'int16': 'int16_t',
    'int32': 'int32_t',
    'int64': 'int64_t',
    'uint8': 'uint8_t',
    'uint16': 'uint16_t',
    'uint32': 'uint32_t',
    'uint64': 'uint64_t',
    'float': 'float',
    'double': 'double',
    'string': 'std::string_view',
    'template': 'template'
}


type_sizes = {
    'int8_t': 1,
    'int16_t': 2,
    'int32_t': 4,
    'int64_t': 8,
    'uint8_t': 1,
    'uint16_t': 2,
    'uint32_t': 4,
    'uint64_t': 8,
    'float': 4,
    'double': 8,
}


field_regex = re.compile(r'^([_A-Za-z0-9]+)(#)?$')


def get_namespaces(data):
    namespaces = []

    for namespace_name in data:
        namespace = data[namespace_name]

        if isinstance(namespace, dict) == False:
            raise RuntimeError('%s: namespace must be a dict' % (namespace_name))

        namespaces.append((namespace_name, namespace))

    return namespaces


def get_structs(data):
    structs = []

    for struct_name in data:
        struct = data[struct_name]

        if struct_name == u'template':
            raise RuntimeError('template is a reserved keyword')

        if isinstance(struct, dict) == False:
            raise RuntimeError('%s: struct must be a dict' % (struct_name))

        structs.append((struct_name, struct))

    return structs


def get_fields(data):
    Field = namedtuple('Field', 'name type is_static is_native is_list')

    fields = []

    for field_name in data:
        field = data[field_name]

        field_is_list = False

        if isinstance(field, str) == True:
            pass

        elif isinstance(field, list) == True:
            if len(field) != 1:
                raise RuntimeError('%s: list must declare a single type' % (field_name))

            field = field[0]

            if isinstance(field, str) == False:
                raise RuntimeError('%s: list type must be a string' % (field_name))

            field_is_list = True

        else:
            raise RuntimeError('%s: invalid type' % (field_name))

        m = field_regex.match(field)

        if m == None:
            raise RuntimeError('%s: invalid type' % (field_name))

        field_type = m.group(1)

        field_is_native = field_type in native_types

        if field_type == 'template':
            field_is_native = False

        if field_is_native == True:
            field_type = native_types[field_type]

        field_is_static = False

        if m.group(2) != None:
            if field_is_list == True:
                raise RuntimeError('%s: lists cannot have static modifier' % (field_name))

            if field_is_native == False:
                raise RuntimeError('%s: static modifier can be applied to native types only' %
                                   field_name)

            if field_type == 'std::string_view':
                raise RuntimeError('%s: static modifier cannot be applied to strings' %
                                   field_name)

            field_is_static = True

        fields.append(Field(field_name,
                            field_type,
                            field_is_static,
                            field_is_native,
                            field_is_list))

    return fields


def get_modules(data):
    modules = []

    for module_name in data:
        module = data[module_name]

        if isinstance(module, dict) == False:
            raise RuntimeError('%s: module must be a dict' % (module_name))

        modules.append((module_name, module))

    return modules


def list_builder_generator(list):
    list_builder_template = '''    struct {{name}}_builder final : public tyrtech::message::list_builder
    {
        {{name}}_builder(tyrtech::message::builder* builder)
          : list_builder(builder)
        {
        }

{% if is_native == True %}
        void add_value(const {{type}}& value)
{% else %}
        decltype(auto) add_value()
{% endif %}
        {
            add_element();
{% if is_native == True %}
            list_builder::add_value(value);
{% else %}
{% if type == 'template' %}
            return m_builder;
{% else %}
            return {{type}}_builder(m_builder);
{% endif %}
{% endif %}
        }
    };'''


    t = jinja2.Template(list_builder_template, trim_blocks=True)
    return t.render(name=list.name,
                    type=list.type,
                    is_native=list.is_native)


def list_parser_generator(list):
    list_parser_template = '''    struct {{name}}_parser final : public tyrtech::message::list_parser
    {
        {{name}}_parser(const tyrtech::message::parser* parser, uint16_t offset)
          : list_parser(parser, offset)
        {
        }

        bool next()
        {
            if (m_elements == 0)
            {
                return false;
            }

            m_elements--;

            m_offset += m_element_size;

{% if is_native == True and type != 'std::string_view' %}
            m_element_size = tyrtech::message::element<{{type}}>::size;
{% else %}
            m_element_size = tyrtech::message::element<uint16_t>().parse(m_parser, m_offset);
            m_element_size += tyrtech::message::element<uint16_t>::size;
{% endif %}

            return true;
        }

        decltype(auto) value() const
        {
{% if is_native == True %}
            return tyrtech::message::element<{{type}}>().parse(m_parser, m_offset);
{% else %}
{% if type == 'template' %}
            return m_offset;
{% else %}
            return {{type}}_parser(m_parser, m_offset);
{% endif %}
{% endif %}
        }
    };'''


    t = jinja2.Template(list_parser_template, trim_blocks=True)
    return t.render(name=list.name,
                    type=list.type,
                    is_native=list.is_native)


def struct_builder_generator(struct):
    struct_builder_template = '''struct {{name}}_builder final : public tyrtech::message::struct_builder<{{elements}}, {{static_size}}>
{
{% for list in lists %}
{{list}}

{% endfor %}
    {{name}}_builder(tyrtech::message::builder* builder)
      : struct_builder(builder)
    {
    }
{% for field in fields %}

{% if field.is_static == False %}
{% if field.is_native == True and field.is_list == False %}
    void add_{{field.name}}(const {{field.type}}& value)
{% else %}
    decltype(auto) add_{{field.name}}()
{% endif %}
    {
        set_offset<{{field_indexes[field.name]}}>();
{% if field.is_native == True and field.is_list == False %}
        struct_builder<{{elements}}, {{static_size}}>::add_value(value);
{% else %}
{% if field.type == 'template' %}
        return m_builder;
{% else %}
{% if field.is_list == True %}
        return {{field.name}}_builder(m_builder);
{% else %}
        return {{field.type}}_builder(m_builder);
{% endif %}
{% endif %}
{% endif %}
    }
{% if field.is_native == True or field.is_list == True %}

    static constexpr uint16_t {{field.name}}_bytes_required()
    {
{% if field.is_list == False %}
        return tyrtech::message::element<{{field.type}}>::size;
{% else %}
        return {{field.name}}_builder::bytes_required();
{% endif %}
    }
{% endif %}
{% else %}
    void set_{{field.name}}({{field.type}} value)
    {
        *reinterpret_cast<{{field.type}}*>(m_static + {{static_fields[field.name]}}) = value;
    }
{% endif %}
{% endfor %}
};'''


    lists = []

    field_indexes = {}

    static_fields = {}
    static_size = 0

    for field in struct.fields:
        if field.is_list:
            lists.append(list_builder_generator(field))

        if field.is_static == True:
            static_fields[field.name] = static_size
            static_size += type_sizes[field.type]

        else:
            next_ndx = len(field_indexes)
            field_indexes[field.name] = next_ndx

    t = jinja2.Template(struct_builder_template, trim_blocks=True)
    return t.render(name=struct.name,
                    elements=len(struct.fields) - len(static_fields),
                    static_size=static_size,
                    static_fields=static_fields,
                    field_indexes=field_indexes,
                    lists=lists,
                    fields=struct.fields)


def struct_parser_generator(struct):
    struct_parser_template = '''struct {{name}}_parser final : public tyrtech::message::struct_parser<{{elements}}, {{static_size}}>
{
{% for list in lists %}
{{list}}

{% endfor %}
    {{name}}_parser(const tyrtech::message::parser* parser, uint16_t offset)
      : struct_parser(parser, offset)
    {
    }

    {{name}}_parser() = default;
{% for field in fields %}

{% if field.is_static == False %}
    bool has_{{field.name}}() const
    {
        return has_offset<{{field_indexes[field.name]}}>();
    }

    decltype(auto) {{field.name}}() const
    {
{% if field.is_native == True and field.is_list == False %}
        return tyrtech::message::element<{{field.type}}>().parse(m_parser, offset<{{field_indexes[field.name]}}>());
{% else %}
{% if field.type == 'template' %}
        return offset<{{field_indexes[field.name]}}>();
{% else %}
{% if field.is_list == True %}
        return {{field.name}}_parser(m_parser, offset<{{field_indexes[field.name]}}>());
{% else %}
        return {{field.type}}_parser(m_parser, offset<{{field_indexes[field.name]}}>());
{% endif %}
{% endif %}
{% endif %}
    }
{% else %}
    decltype(auto) {{field.name}}() const
    {
        return *reinterpret_cast<const {{field.type}}*>(m_static + {{static_fields[field.name]}});
    }
{% endif %}
{% endfor %}
};'''


    lists = []

    field_indexes = {}

    static_fields = {}
    static_size = 0

    for field in struct.fields:
        if field.is_list:
            lists.append(list_parser_generator(field))

        if field.is_static == True:
            static_fields[field.name] = static_size
            static_size += type_sizes[field.type]

        else:
            next_ndx = len(field_indexes)
            field_indexes[field.name] = next_ndx

    t = jinja2.Template(struct_parser_template, trim_blocks=True)
    return t.render(name=struct.name,
                    elements=len(struct.fields) - len(static_fields),
                    static_size=static_size,
                    static_fields=static_fields,
                    field_indexes=field_indexes,
                    lists=lists,
                    fields=struct.fields)


def struct_generator(name, struct):
    Struct = namedtuple('Struct', 'name fields')

    fields = get_fields(struct)

    return (struct_builder_generator(Struct(name, fields)),
            struct_parser_generator(Struct(name, fields)))


def messages_generator(data, output):
    messages_template = '''#pragma once


#include <message/builder.h>
#include <message/parser.h>
{% for namespace in namespaces %}


namespace {{namespace.name}} {

{% for struct in namespace.structs %}

{{struct}}
{% endfor %}

}{% endfor %}'''


    Namespace = namedtuple('Namespace', 'name structs')

    namespaces = []

    for n_name, n in get_namespaces(data):
        structs = []

        for s_name, s in get_structs(n):
            structs.extend(struct_generator(s_name, s))

        namespaces.append(Namespace(n_name, structs))

    t = jinja2.Template(messages_template, trim_blocks=True)
    print(t.render(namespaces=namespaces), file=output)


def func_messages_generator(data, name):
    func_messages_template = '''namespace messages::{{func_name}} {

{% for func in funcs %}

{{ func }}
{% endfor %}

}'''


    funcs = []

    for f_name in data:
        if f_name == 'id':
            continue

        struct = data[f_name]

        if isinstance(struct, dict) == False:
            raise RuntimeError('%s: invalid struct', f_name)

        funcs.extend(struct_generator(f_name, struct))

    t = jinja2.Template(func_messages_template, trim_blocks=True)
    return t.render(func_name=name, funcs=funcs)


def func_definition_generator(func, func_name, module_id, module_name):
    func_definition_template = '''struct {{func_name}}
{
    static constexpr uint16_t id{{'{'}}{{func_id}}{{'}'}};
    static constexpr uint16_t module_id{{'{'}}{{module_id}}{{'}'}};

    using request_builder_t =
            messages::{{func_name}}::request_builder;

    using request_parser_t =
            messages::{{func_name}}::request_parser;

    using response_builder_t =
            messages::{{func_name}}::response_builder;

    using response_parser_t =
            messages::{{func_name}}::response_parser;

    static void throw_exception(const tyrtech::net::service::error_parser& error)
    {
        throw_module_exception(error);
    }
};'''


    func_id = func['id']

    if isinstance(func_id, int) == False:
        raise RuntimeError('id: must be an integer')

    t = jinja2.Template(func_definition_template, trim_blocks=True)
    return t.render(module_name=module_name,
                    module_id=module_id,
                    func_name=func_name,
                    func_id=func_id)


def module_generator(module_name, module, namespace):
    module_template = '''namespace {{namespace}}::{{module_name}} {

{% for func_msg in func_msgs %}

{{func_msg}}
{% endfor %}

{% for exception in exceptions %}
DEFINE_SERVER_EXCEPTION({{loop.index}}, tyrtech::net::server_error, {{exception}});
{% if loop.last == True %}{{'\n'}}{% endif %}
{% endfor %}
void throw_module_exception(const tyrtech::net::service::error_parser& error)
{
    switch (error.code())
    {
        case -1:
        {
            throw tyrtech::net::unknown_module_error("{}", error.message());
        }
        case -2:
        {
            throw tyrtech::net::unknown_function_error("{}", error.message());
        }
{% for exception in exceptions %}
        case {{loop.index}}:
        {
            throw {{exception}}("{{'{'}}{{'}'}}", error.message());
        }
{% endfor %}
        default:
        {
            throw tyrtech::net::unknown_exception_error("#{}: unknown exception", error.code());
        }
    }
}

static constexpr uint16_t id{{'{'}}{{module_id}}{{'}'}};
{% for func in func_defs %}

{{func}}
{% endfor %}

template<typename Implementation>
struct module : private tyrtech::disallow_copy
{
    Implementation* impl{nullptr};

    module(Implementation* impl)
      : impl(impl)
    {
    }

    void process_message(const tyrtech::net::service::request_parser& service_request,
                         tyrtech::net::service::response_builder* service_response,
                         typename Implementation::context* ctx)
    {
        switch (service_request.function())
        {
{% for func in funcs %}
            case {{func.name}}::id:
            {
                using request_parser_t =
                        typename {{func.name}}::request_parser_t;

                using response_builder_t =
                        typename {{func.name}}::response_builder_t;

                request_parser_t request(service_request.get_parser(),
                                         service_request.message());
                response_builder_t response(service_response->add_message());

                impl->{{func.name}}(request, &response, ctx);

                break;
            }
{% endfor %}
            default:
            {
                throw tyrtech::net::unknown_function_error("#{}: unknown function", service_request.function());
            }
        }
    }

    decltype(auto) create_context(const std::shared_ptr<tyrtech::io::channel>& remote)
    {
        return impl->create_context(remote);
    }
};

}'''


    reserved_keys = set(['id', 'exceptions'])

    Function = namedtuple('Function', 'name id')

    module_id = module['id']
    exceptions = module['exceptions']
    func_msgs = []
    func_defs = []
    funcs = []

    if isinstance(module_id, int) == False:
        raise RuntimeError('id: must be an integer')

    if isinstance(exceptions, list) == False:
        raise RuntimeError('exceptions: must be a list of strings')

    for exception in exceptions:
        if isinstance(exception, str) == False:
            raise RuntimeError('exceptions: must be a list of strings')

    for func_name in module:
        if func_name in reserved_keys:
            continue

        func = module[func_name]

        if isinstance(func, dict) == False:
            raise RuntimeError('%s: must be a dict' % (func_name))

        func_msgs.append(func_messages_generator(func, func_name))
        func_defs.append(func_definition_generator(module[func_name], func_name, module_id, module_name))
        funcs.append(Function(func_name, func['id']))

    t = jinja2.Template(module_template, trim_blocks=True)
    return t.render(module_name=module_name,
                    module_id=module_id,
                    funcs=funcs,
                    func_msgs=func_msgs,
                    func_defs=func_defs,
                    exceptions=exceptions,
                    namespace=namespace)


def modules_generator(data, output):
    modules_template = '''#pragma once


#include <io/channel.h>
#include <net/server_exception.h>
#include <net/service.json.h>

{% for module in modules %}

{{module}}{% endfor %}'''


    Namespace = namedtuple('Namespace', 'name modules')

    modules = []

    for n_name, n in get_namespaces(data):
        for m_name, m in get_modules(n):
            modules.append(module_generator(m_name, m, n_name))

    t = jinja2.Template(modules_template, trim_blocks=True)
    print(t.render(modules=modules), file=output)


def services_generator(data, output):
    services_template = '''#pragma once


#include <io/channel.h>
#include <net/server_exception.h>
#include <net/service.json.h>

{% for include in includes %}
#include <{{include}}>
{% endfor %}
{% for namespace in namespaces %}


namespace {{namespace.name}} {


{% for service in namespace.services %}
template<
{% for module in service.modules %}
    typename {{module}}Impl{% if not loop.last %},{% endif %}

{% endfor %}
>
struct {{service.name}} : private tyrtech::disallow_copy
{
{% for module in service.modules %}
    {{module}}::module<{{module}}Impl> {{module}};
{% endfor %}

    {{service.name}}(
{% for module in service.modules %}
        {{module}}Impl* {{module}}{% if not loop.last %},{% endif %}

{% endfor %}
    )
{% for module in service.modules %}
      {% if loop.first %}: {% else %}, {% endif %}{{module}}({{module}})
{% endfor %}
    {
    }

    struct context : private tyrtech::disallow_copy
    {
{% for module in service.modules %}
        typename {{module}}Impl::context {{module}}_ctx;
{% endfor %}

        context(
{% for module in service.modules %}
            typename {{module}}Impl::context&& {{module}}_ctx{% if not loop.last %},{% endif %}

{% endfor %}
        )
{% for module in service.modules %}
          {% if loop.first %}: {% else %}, {% endif %}{{module}}_ctx(std::move({{module}}_ctx))
{% endfor %}
        {
        }
    };

    context create_context(const std::shared_ptr<tyrtech::io::channel>& remote)
    {
        return context(
{% for module in service.modules %}
            {{module}}.create_context(remote){% if not loop.last %},{% endif %}

{% endfor %}
        );
    }

    void process_message(const tyrtech::net::service::request_parser& service_request,
                         tyrtech::net::service::response_builder* service_response,
                         context* ctx)
    {
        switch (service_request.module())
        {
{% for module in service.modules %}
            case {{module}}::id:
            {
                {{module}}.process_message(service_request, service_response, &ctx->{{module}}_ctx);

                break;
            }
{% endfor %}
            default:
            {
                throw tyrtech::net::unknown_module_error("#{}: unknown module", service_request.function());
            }
        }
    }
};

{% endfor %}
}{% endfor %}'''


    Namespace = namedtuple('Namespace', 'name services')
    Service = namedtuple('Service', 'name modules')

    includes = []
    namespaces = []

    for element_name in data:
        element = data[element_name]

        if element_name == 'includes':
            if isinstance(element, list) == False:
                raise RuntimeError('includes: must be a list of strings')

            for include in element:
                if isinstance(include, str) == False:
                    raise RuntimeError('includes: must be a list of strings')

                includes.append(include)

        else:
            if isinstance(element, dict) == False:
                raise RuntimeError('%s: invalid type' % (element_name))

            services = []

            for service_name in element:
                service = element[service_name]

                modules = []

                if isinstance(service, list) == False:
                    raise RuntimeError('%s: must be a list of strings' % (service_name))

                for module in service:
                    if isinstance(module, str) == False:
                        raise RuntimeError('%s: must be a list of strings' % (service_name))

                    modules.append(module)

                services.append(Service(service_name, modules))

            namespaces.append(Namespace(element_name, services))

    t = jinja2.Template(services_template, trim_blocks=True)
    print(t.render(includes=includes, namespaces=namespaces), file=output)


def main():
    p = argparse.ArgumentParser(description='generate message/rpc definitions')

    g = p.add_mutually_exclusive_group(required=True)
    g.add_argument('--services', action='store_true', help='generate services definitions')
    g.add_argument('--modules', action='store_true', help='generate modules definitions')
    g.add_argument('--messages', action='store_true', help='generate message definitions')
    p.add_argument('INPUT', help='input file')

    args = vars(p.parse_args())

    input_file = args['INPUT']
    output_file = '%s.h' % (input_file,)

    data = json.loads(open(input_file).read(), object_pairs_hook=OrderedDict)

    output = open(output_file, 'w')

    if args['messages'] == True:
        messages_generator(data, output)

    if args['modules'] == True:
        modules_generator(data, output)

    if args['services'] == True:
        services_generator(data, output)


if __name__ == '__main__':
    main()
