/*
(C) 2022, Andrea Boni
This file is part of n2k_battery_monitor.
n2k_battery_monitor is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
NMEARouter is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with n2k_battery_monitor.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "VeDirect.h"
#include "Utils.h"
#include <stdio.h>
#include <string.h>

/*
VE.Direct format:

PID     0xA381
V       12488
VS      12909
I       0
P       0
CE      -220375
SOC     196
TTG     -1
Alarm   ON
Relay   OFF
AR      4
BMV     712 Smart
FW      0408
MON     0

FMT Line:
/r/n<name>/t<value>

00000000  0d 0a 50 49 44 09 30 78  32 30 33 0d 0a 56 09 32  |..PID.0x203..V.2|
00000010  36 32 30 31 0d 0a 49 09  30 0d 0a 50 09 30 0d 0a  |6201..I.0..P.0..|
00000020  43 45 09 30 0d 0a 53 4f  43 09 31 30 30 30 0d 0a  |CE.0..SOC.1000..|
00000030  54 54 47 09 2d 31 0d 0a  41 6c 61 72 6d 09 4f 46  |TTG.-1..Alarm.OF|
00000040  46 0d 0a 52 65 6c 61 79  09 4f 46 46 0d 0a 41 52  |F..Relay.OFF..AR|
00000050  09 30 0d 0a 42 4d 56 09  37 30 30 0d 0a 46 57 09  |.0..BMV.700..FW.|
00000060  30 33 30 37 0d 0a 43 68  65 63 6b 73 75 6d 09 d8  |0307..Checksum..|

/*
PID	0xA381
V	13406
VS	13152
I	0
P	0
CE	-89423
SOC	689
TTG	-1
Alarm	OFF
Relay	OFF
AR	0
BMV	712 Smart
FW	0413
MON	0
Checksum	y
H1	-277191
H2	-89430
H3	-137695
H4	21
H5	1
H6	-5966596
H7	30
H8	16200
H9	86935
H10	17
H11	71
H12	0
H15	22
H16	15394
H17	7749
H18	9056
Checksum

 */

static const char *EMPTY_STRING = "";

bool start_with_unsafe(const char *test_str, const char* string)
{
    int i = 0;
    do
    {
        if (string[i]!=test_str[i] || string[i]==0) return false;
        i++;
    } while (test_str[i]);
    return true;
}

int _read_vedirect(char *output, const char *tag, const char *line)
{
    char str[80];
    strcpy(str, line);
    char *token;
    token = strtok(str, "\t");
    if (token && strcmp(tag, token) == 0)
    {
        token = strtok(NULL, "\t");
        if (token)
        {
            strcpy(output, token);
            return -1;
        }
    }
    return 0;
}

int read_vedirect_int(int &v, const char *tag, const char *line)
{
    char token[80];
    if (_read_vedirect(token, tag, line))
    {
        if (strcmp("---", token) == 0)
        {
            return 0; // undefined value in ve.direct dialect
        }
        else
        {
            v = strtol(token, NULL, 0);
            return -1;
        }
    }
    return 0;
}

int read_vedirect(double &v, double precision, const char *tag, const char *line)
{
    int iv = 0;
    if (read_vedirect_int(iv, tag, line))
    {
        v = iv * precision;
        return -1;
    }
    return 0;
}

int read_vedirect_onoff(bool &v, const char *tag, const char *line)
{
    char token[80];
    if (_read_vedirect(token, tag, line))
    {
        v = strcmp("ON", token);
        return -1;
    }
    return 0;
}

VEDirectField::VEDirectField(const VEDirectValueDefinition& d): def(d), value_set(false), last_time(0)
{}

const VEDirectValueDefinition& VEDirectField::get_definition()
{
    return def;
}

unsigned long VEDirectField::get_last_time()
{
    return last_time;
}

bool VEDirectField::is_set()
{
    return value_set;
}

void VEDirectField::set_last_time(unsigned long t)
{
    last_time = t;
}

void VEDirectField::set_value_set(bool b)
{
    value_set = b;
}

void VEDirectField::unset()
{
    value_set = false;
}

VEDirectFieldNumber::VEDirectFieldNumber(const VEDirectValueDefinition& d): VEDirectField(d)
{}

int VEDirectFieldNumber::get_value()
{
    return value;
}

void VEDirectFieldNumber::set_value(int v)
{
    value = v;
    set_value_set(true);
}

bool VEDirectFieldNumber::parse(const char* v)
{
    if (start_with_unsafe("0x", v))
        set_value(strtol(v, 0, 16));
    else
        set_value(atoi(v));
    return true;
}

VEDirectFieldBool::VEDirectFieldBool(const VEDirectValueDefinition& d): VEDirectField(d)
{}

bool VEDirectFieldBool::get_value()
{
    return value;
}

void VEDirectFieldBool::set_value(bool v)
{
    value = v;
    set_value_set(true);
}

bool VEDirectFieldBool::parse(const char* v)
{
    set_value(strcmp("ON", v)==0);
    return true;
}

VEDirectFieldString::VEDirectFieldString(const VEDirectValueDefinition& d): VEDirectField(d)
{}

const char* VEDirectFieldString::get_value()
{
    return value;
}

void VEDirectFieldString::set_value(const char* v)
{
    strlcpy(value, v, 16);
    set_value_set(true);
}

bool VEDirectFieldString::parse(const char* v)
{
    set_value(v);
    return true;
}

VEDirectObject::VEDirectObject() : valid(0), n_fields(0), checksum(0), listener(nullptr), values(nullptr)
{}

void VEDirectObject::init(const VEDirectValueDefinition *definition, unsigned int n)
{
    n_fields = n;
    values = new VEDirectField*[n_fields];
    for (int i = 0; i<n_fields; i++)
    {
        switch (definition[i].veType)
        {
            case VE_BOOLEAN:
                values[i] = new VEDirectFieldBool(definition[i]);
                break;
            case VE_NUMBER:
            case VE_HEX:
                values[i] = new VEDirectFieldNumber(definition[i]);
                break;
            case VE_STRING:
            default:
                values[i] = new VEDirectFieldString(definition[i]);
                break;
        }
    }
    reset();
}

VEDirectObject::~VEDirectObject()
{
    for (int i = 0; i < n_fields; i++)
    {
        if (values[i]) delete values[i];
    }
    delete values;
}

void VEDirectObject::set_listener(VEDirectListener *l)
{
    listener = l;
}

void VEDirectObject::reset()
{
    for (int i = 0; i < n_fields; i++)
    {
        values[i]->set_last_time(0);
        values[i]->unset();
    }
    valid = 0;
    checksum = 0;
}

void VEDirectObject::print()
{
    /*Log::trace("New ve.direct object\n");
    for (int i = 0; i < n_fields; i++)
    {
        if (last_time[i])
            switch (fields[i].veType)
            {
            case VE_BOOLEAN:
                Log::trace("Field %d %s {%s}\n", i, fields[i].veName, i_values[i] ? "ON" : "OFF");
                break;
            case VE_NUMBER:
                if (fields[i].veUnit)
                    Log::trace("Field %d %s {%d %s}\n", i, fields[i].veName, i_values[i], fields[i].veUnit);
                else
                    Log::trace("Field %d %s {%d}\n", i, fields[i].veName, i_values[i]);
                break;
            case VE_STRING:
                Log::trace("Field %d %s {%s}\n", i, fields[i].veName, s_values[i]);
                break;
            default:
                break;
            }
    }
    Log::trace("End ve.direct object\n");*/
}

bool load_key_value(const char* line, char* key, int max_key_len, char* value, int max_value_len)
{
    key[0] = 0;
    value[0] = 0;
    int tab = -1;
    for (int i = 0; line[i]; i++)
    {
        if (line[i] == '\t')
        {
            tab = i;
        }
        else if (tab!=-1)
        {
            int ix = i - tab;
            if (ix == max_value_len) return false; // value too long
            value[ix - 1] = line[i];
            value[ix] = 0;
        }
        else
        {
            if ((i + 1) == max_key_len) return false; // key too long
            key[i] = line[i];
            key[i + 1] = 0;
        }
    }
    return key[0] && value[0];
}

int get_field_def(VEDirectField** vs, int array_size, const char* key)
{
    for (int i = 0; i<array_size; i++)
        if (vs[i] && strcmp(key, vs[i]->get_definition().veName)==0) return i;
    return -1;
}

void VEDirectObject::load_VEDirect_key_value(const char *line, unsigned long time)
{
    char key[16];
    char value[16];
    if (!load_key_value(line, key, 16, value, 16)) return;
    //printf("Key {%s} Value {%s}\n", key, value);

    int field_ix = get_field_def(values, n_fields, key);
    if (field_ix==-1) return;
    //printf("Match key {%s} index {%d} ", key, i);

    if (values[field_ix]->parse(value)) valid++;
}

int VEDirectObject::get_number_value(int &value, unsigned int index)
{
    if (index > n_fields)
    {
        //printf("Index %d out of range %d\n", index, n_fields);
        return 0;
    }
    VEDirectValueDefinition field = values[index]->get_definition();
    if (field.veIndex < BMV_N_FIELDS && (field.veType==VE_NUMBER || field.veType==VE_HEX) /*&& last_time[field.veIndex]*/)
    {
        value = ((VEDirectFieldNumber*)values[field.veIndex])->get_value();
        return -1;
    }
    else
    {
        return 0;
    }
}

int VEDirectObject::get_number_value(double &value, double precision, unsigned int index)
{
    if (index > n_fields)
    {
        //printf("Index %d out of range %d\n", index, n_fields);
        return 0;
    }
    VEDirectValueDefinition field = values[index]->get_definition();
    if (field.veType==VE_NUMBER /*&& last_time[field.veIndex]*/)
    {
        value = ((VEDirectFieldNumber*)values[field.veIndex])->get_value() * precision;
        return -1;
    }
    else
    {
        return 0;
    }
}

int VEDirectObject::get_boolean_value(bool &value, unsigned int index)
{
    if (index > n_fields)
        return 0;
    VEDirectValueDefinition field = values[index]->get_definition();
    if (field.veType==VE_BOOLEAN/* && last_time[field.veIndex]*/)
    {
        value = ((VEDirectFieldBool*)values[field.veIndex])->get_value();
        return -1;
    }
    else
    {
        return 0;
    }
}

int VEDirectObject::get_string_value(char *value, unsigned int index)
{
    if (index > n_fields)
        return 0;
    VEDirectValueDefinition field = values[index]->get_definition();
    if (field.veType==VE_STRING/* && last_time[field.veIndex]*/)
    {
        strcpy(value, ((VEDirectFieldString*)values[field.veIndex])->get_value());
        return -1;
    }
    else
    {
        return 0;
    }
}

unsigned long VEDirectObject::get_last_timestamp(unsigned int index)
{
    if (index > n_fields)
        return 0;
    return values[index]->get_last_time();
}

bool VEDirectObject::is_valid()
{
    return valid && checksum == 0;
}

int update_checksum(int checksum, const char *line)
{
    uint8_t c;
    int i = 0;
    checksum = (checksum + '\n') & 0xFF;
    checksum = (checksum + '\r') & 0xFF;
    do
    {
        c = line[i];
        checksum = (checksum + c) & 0xFF;
        i++;
    } while (c);
    return checksum;
}

void VEDirectObject::on_line_read(const char *line)
{
    checksum = update_checksum(checksum, line);
    //printf("Line {%s} ", line);
    if (line[0])
    {
        load_VEDirect_key_value(line, 0);
    }
}

void VEDirectObject::on_partial(const char *line, int len)
{
    static size_t l_checkusm = strlen("Checksum\tx");
    static size_t l_PID = strlen("PID\t");
    if (len == l_checkusm && start_with_unsafe("Checksum", line))
    {
        checksum = update_checksum(checksum, line);
        if (listener) listener->on_complete(*this);
    }
    else if (len == l_PID && start_with_unsafe("PID", line))
    {
        reset();
    }
}