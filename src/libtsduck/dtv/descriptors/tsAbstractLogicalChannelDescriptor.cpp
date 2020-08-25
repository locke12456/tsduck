//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsAbstractLogicalChannelDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AbstractLogicalChannelDescriptor::AbstractLogicalChannelDescriptor(DID tag,
                                                                       const UChar* xml_name,
                                                                       Standards standards,
                                                                       PDS pds,
                                                                       const UChar* xml_legacy_name) :
    AbstractDescriptor(tag, xml_name, standards, pds, xml_legacy_name),
    entries()
{
}

void ts::AbstractLogicalChannelDescriptor::clearContent()
{
    entries.clear();
}

ts::AbstractLogicalChannelDescriptor::AbstractLogicalChannelDescriptor(DuckContext& duck,
                                                                       const Descriptor& desc,
                                                                       DID tag,
                                                                       const UChar* xml_name,
                                                                       Standards standards,
                                                                       PDS pds,
                                                                       const UChar* xml_legacy_name) :
    AbstractLogicalChannelDescriptor(tag, xml_name, standards, pds, xml_legacy_name)
{
    deserialize(duck, desc);
}

ts::AbstractLogicalChannelDescriptor::Entry::Entry(uint16_t i, bool v, uint16_t l):
    service_id(i),
    visible(v),
    lcn(l)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AbstractLogicalChannelDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        bbp->appendUInt16(it->service_id);
        bbp->appendUInt16((it->visible ? 0xFC00 : 0x7C00) | (it->lcn & 0x03FF));
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AbstractLogicalChannelDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() % 4 == 0;
    entries.clear();

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        while (size >= 4) {
            entries.push_back(Entry(GetUInt16(data), (data[2] & 0x80) != 0, GetUInt16(data + 2) & 0x03FF));
            data += 4;
            size -= 4;
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AbstractLogicalChannelDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const UString margin(indent, ' ');

    while (size >= 4) {
        const uint16_t service = GetUInt16(data);
        const uint8_t visible = (data[2] >> 7) & 0x01;
        const uint16_t channel = GetUInt16(data + 2) & 0x03FF;
        data += 4; size -= 4;
        strm << margin
             << UString::Format(u"Service Id: %5d (0x%04X), Visible: %1d, Channel number: %3d", {service, service, visible, channel})
             << std::endl;
    }

    display.displayExtraData(data, size, margin);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AbstractLogicalChannelDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        xml::Element* e = root->addElement(u"service");
        e->setIntAttribute(u"service_id", it->service_id, true);
        e->setIntAttribute(u"logical_channel_number", it->lcn, false);
        e->setBoolAttribute(u"visible_service", it->visible);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AbstractLogicalChannelDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"service", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getIntAttribute<uint16_t>(entry.service_id, u"service_id", true, 0, 0x0000, 0xFFFF) &&
             children[i]->getIntAttribute<uint16_t>(entry.lcn, u"logical_channel_number", true, 0, 0x0000, 0x03FF) &&
             children[i]->getBoolAttribute(entry.visible, u"visible_service", false, true);
        if (ok) {
            entries.push_back(entry);
        }
    }
    return ok;
}
