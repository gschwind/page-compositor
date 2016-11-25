#!/usr/bin/env python3
#-*- coding: utf-8 -*-

import re, os, sys
import xml.etree.ElementTree as ET
import argparse

r_invalid = re.compile('(\W+)')

#parser = argparse.ArgumentParser(description='Generate wayland API C++ to C wrapper')
#parser.add_argument('integers', metavar='N', type=int, nargs='+',
#        help='an integer for the accumulator')
#parser.add_argument('--sum', dest='accumulate', action='store_const',
#        const=sum, default=max,
#        help='sum the integers (default: find the max)')
#
#    args = parser.parse_args()
#print(args.accumulate(args.integers))

def gen_args_with_type(args, args_base = []):
 arglist = args_base
 for arg in args:
  aname = arg.attrib['name']
  atype = arg.attrib['type']
  if atype != 'object':
   arglist.append('{0} {1}'.format(maptype[atype], aname))
  else:
   arglist.append('struct {0} * {1}'.format(arg.attrib['interface'], aname))
 return ', '.join(arglist)

def gen_args(args, args_base = []):
 arglist = args_base
 for arg in args:
  aname = arg.attrib['name']
  arglist.append(aname)
 return ', '.join(arglist)


maptype = {
 'uint': 'uint32_t',
 'int': 'int32_t',
 'new_id': 'uint32_t',
 'string': 'const char *',
 'object': None
}

def gen_header(fi_name, fo):
 fi_xname = os.path.basename(fi_name)[:-4]
 fi_uname = r_invalid.sub('_', fi_xname).upper()
 tree = ET.parse(fi_name)
 root = tree.getroot()
 fo.write("""
/*
 * Copyright (2016) Benoit Gschwind
 *
 * This file is part of page-compositor.
 *
 * page-compositor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * page-compositor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with page-compositor.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
""")
 
 fo.write("""
#ifndef WCXX_{UNAME}_HXX_
#define WCXX_{UNAME}_HXX_

#include <libweston-0/compositor.h>

namespace wcxx {{
""".format(UNAME = fi_uname))
 
 for interface in root.findall('interface'):
  interface_name = interface.attrib['name']
  fo.write("struct {0}_vtable {{\n".format(interface_name))
  fo.write("\tvirtual ~{0}() = default;\n".format(interface_name))
  funclist = []
  for request in interface.findall('request'):
   args = request.findall('arg')
   fo.write('\tvirtual void {0}_{1}({2}) = 0;\n'.format(interface_name, request.attrib['name'], gen_args_with_type(args, ['struct wl_client * client', 'struct wl_resource * resource'])))
  fo.write('\tvirtual void {0}_delete_resource(struct wl_resource * resource) = 0;\n'.format(interface_name))
  fo.write('\tvoid set_implementation(struct wl_resource * resource);\n')
  fo.write('};\n')
 fo.write("}}\n#endif /* WCXX_{UNAME}_HXX_ */\n".format(UNAME = fi_uname))

def gen_impl(fi_name, fo):
 fi_xname = os.path.basename(fi_name)[:-4]
 fi_uname = r_invalid.sub('_', fi_xname).upper()
 tree = ET.parse(fi_name)
 root = tree.getroot()
 fo.write("""
/*
 * Copyright (2016) Benoit Gschwind
 *
 * This file is part of page-compositor.
 *
 * page-compositor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * page-compositor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with page-compositor.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "{XNAME}-interface.hxx"
#include "{XNAME}-server-protocol.h"

namespace wcxx {{

namespace hidden {{
""".format(XNAME=fi_xname))
 
 for interface in root.findall('interface'):
  interface_name = interface.attrib['name']
  fo.write('inline {0}_vtable * {0}_get(struct wl_resource * resource) {{\n'.format(interface_name))
  fo.write('\treturn reinterpret_cast<{0}_vtable *>(wl_resource_get_user_data(resource));\n'.format(interface_name))
  fo.write('}\n\n')
  for request in interface.findall('request'):
   args = request.findall('arg')
   args_with_type = gen_args_with_type(args, ['struct wl_client * client', 'struct wl_resource * resource'])
   args_no_type = gen_args(args, ['client', 'resource'])
   fo.write('void {0}_{1}({2}) {{\n'.format(interface_name, request.attrib['name'], args_with_type))
   fo.write('\t{0}_get(resource)->{0}_{1}({2});\n'.format(interface_name, request.attrib['name'], args_no_type))
   fo.write('}\n\n')
  fo.write('void {0}_delete_resource({1}) {{\n'.format(interface_name, args_with_type))
  fo.write('\t{0}_get(resource)->{0}_delete_resource({1});\n'.format(interface_name, args_no_type))
  fo.write('}\n\n')
  fo.write('static struct {0}_interface const {0}_implementation = {{\n'.format(interface_name))
  requests = interface.findall('request')
  for request in requests[:-1]:
   fo.write('\t{0}_{1},\n'.format(interface_name, request.attrib['name']))
  if len(requests) > 0:
   fo.write('\t{0}_{1}\n'.format(interface_name, requests[-1].attrib['name']))
  fo.write('};\n\n')
 fo.write('}\n')
 
 for interface in root.findall('interface'):
  fo.write("""
void {0}_vtable::set_implementation(
    struct wl_resource * resource) {{
wl_resource_set_implementation(resource,
        &hidden::{0}_implementation,
        this, &hidden::{0}_delete_resource);
}}
""".format(interface_name))
 
fi_name = sys.argv[1]
fi_xname = os.path.basename(fi_name)[:-4]

gen_header(fi_name, open(fi_xname+'-interface.hxx', 'w'))
gen_impl(fi_name, open(fi_xname+'-interface.cxx', 'w'))


