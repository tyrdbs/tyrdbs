#!/usr/bin/env python

import sys
import json
import jinja2
import argparse

from collections import namedtuple
from collections import OrderedDict


probes_template = '''#define _SDT_HAS_SEMAPHORES 1
#include <sys/sdt.h>


{% for namespace in namespaces %}
{% for provider in namespace.providers %}
{% for probe in provider.probes %}
__extension__ unsigned short {{provider.name}}_{{probe.name}}_semaphore __attribute__ ((unused)) __attribute__ ((section (".probes"))) __attribute__ ((visibility ("hidden")));
{% endfor %}
{% endfor %}
{% endfor %}

{% for namespace in namespaces %}


namespace {{namespace.name}} {


{% for provider in namespace.providers %}
{% for probe in provider.probes %}

struct {{provider.name}}_{{probe.name}}_probe
{
{% for arg in probe.args %}
    {{arg.type}} {{arg.name}};
{% if loop.last == True %}{{'\n'}}{% endif %}
{% endfor %}
{% if probe.args|length != 0 %}
    {{provider.name}}_{{probe.name}}_probe({% for arg in probe.args %}{% if loop.first == False %}, {% endif %}{% if arg.type.startswith('const ') %}{% else %}const {% endif %}{{arg.type}}& {{arg.name}}{% endfor %})
{% for arg in probe.args %}      {% if loop.first == True %}:{% else %},{% endif %} {{arg.name}}({{arg.name}}){{'\n'}}{% endfor %}
    {
    }

{% endif %}
    {{provider.name}}_{{probe.name}}_probe() noexcept = default;

    void fire() const
    {
        DTRACE_PROBE{% if probe.args|length != 0 %}{{probe.args|length}}{% endif %}({{provider.name}}, {{probe.name}}{% for arg in probe.args %}, {{arg.name}}{% endfor %});
    }

    static inline bool is_enabled()
    {
        return __builtin_expect({{provider.name}}_{{probe.name}}_semaphore, 0);
    }
};
{% endfor %}
{% endfor %}

}{% endfor %}'''


def probes_generator(data, output):
    Namespace = namedtuple('Namespace', 'name providers')
    Provider = namedtuple('Provider', 'name probes')
    Probe = namedtuple('Probe', 'name args')
    Arg = namedtuple('Arg', 'name type')

    namespaces = []

    for namespace_name in data:
        namespace = data[namespace_name]

        if isinstance(namespace, dict) == False:
            raise RuntimeError('%s: invalid type' % (namespace_name))

        providers = []

        for provider_name in namespace:
            provider = namespace[provider_name]

            if isinstance(provider, dict) == False:
                raise RuntimeError('%s: invalid type' % (provider_name))

            probes = []

            for probe_name in provider:
                probe = provider[probe_name]

                if isinstance(probe, dict) == False:
                    raise RuntimeError('%s: invalid type' % (probe_name))

                args = []

                for arg_name in probe:
                    arg = probe[arg_name]

                    if isinstance(arg, str) == False:
                        raise RuntimeError('%s: invalid type' % (arg_name))

                    args.append(Arg(arg_name, arg))

                probes.append(Probe(probe_name, args))

            providers.append(Provider(provider_name, probes))

        namespaces.append(Namespace(namespace_name, providers))

    t = jinja2.Template(probes_template, trim_blocks=True)

    print(t.render(namespaces=namespaces), file=output)


def main():
    p = argparse.ArgumentParser(description='generate probe definitions')

    p.add_argument('INPUT', help='input file')

    args = vars(p.parse_args())

    input_file = args['INPUT']
    output_file = '%s.h' % (input_file,)

    data = json.loads(open(input_file).read(), object_pairs_hook=OrderedDict)

    output = open(output_file, 'w')

    probes_generator(data, output)


if __name__ == '__main__':
    main()
