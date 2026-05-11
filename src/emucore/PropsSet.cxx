//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <algorithm>

#include "bspf.hxx"
#include "FSNode.hxx"
#include "DefProps.hxx"
#include "Props.hxx"
#include "PropsSet.hxx"
#include "repository/CompositeKeyValueRepositoryNoop.hxx"
#include "repository/KeyValueRepositoryPropertyFile.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropertiesSet::PropertiesSet()
  : myRepository{std::make_shared<CompositeKeyValueRepositoryNoop>()}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::setRepository(shared_ptr<CompositeKeyValueRepository> repository)
{
  myRepository = std::move(repository);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PropertiesSet::getMD5(string_view md5, Properties& properties,
                           bool useDefaults) const
{
  properties.setDefaults();
  bool found = false;

  // There are three lists to search when looking for a properties entry,
  // which must be done in the following order.
  // If 'useDefaults' is specified, only use the built-in list.
  //
  //  'save': entries previously inserted that are saved on program exit
  //  'temp': entries previously inserted that are discarded
  //  'builtin': the defaults compiled into the program

  // First check properties from external repository
  if(!useDefaults)
  {
    if(myRepository->has(md5))
    {
      properties.load(*myRepository->get(md5));
      found = true;
    }
    else  // Search temp list
    {
      // std::less<> on the map allows heterogeneous lookup with string_view,
      // so no string construction occurs here
      const auto tmp = myTempProps.find(md5);
      if(tmp != myTempProps.end())
      {
        properties = tmp->second;
        found = true;
      }
    }
  }

  // Otherwise, search the internal database using binary search
  if(!found)
  {
    const auto it = std::ranges::lower_bound( // NOLINT(readability-qualified-auto)
      DefProps, md5,
      [](string_view a, string_view b) {
          return BSPF::compareIgnoreCase(a, b) < 0;
      },
      [](const auto& entry) {
          return entry[static_cast<uInt8>(PropType::Cart_MD5)];
      });

    if(it != DefProps.end() &&
       BSPF::compareIgnoreCase((*it)[static_cast<uInt8>(PropType::Cart_MD5)], md5) == 0)
    {
      const auto& entry = *it;
      for(uInt8 p = 0; p < static_cast<uInt8>(PropType::NumTypes); ++p)
        if(entry[p][0] != 0)
          properties.set(PropType{p}, entry[p]);

      found = true;
    }
  }

  return found;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::insert(const Properties& properties, bool save)
{
  // Note that the following code is optimized for insertion when an item
  // doesn't already exist, and when the external properties file is
  // relatively small (which is the case with current versions of Stella,
  // as the properties are built-in).
  // If an item does exist, it will be removed and insertion done again.
  // This shouldn't be a speed issue, as insertions will only fail with
  // duplicates when you're changing the current ROM properties, which
  // most people tend not to do.

  // Since the PropSet is keyed by md5, we can't insert without a valid one
  const string& md5 = properties.get(PropType::Cart_MD5);
  if(md5.empty())
    return;

  // Make sure the exact entry isn't already in any list
  Properties defaultProps;
  if(getMD5(md5, defaultProps, false) && defaultProps == properties)
    return;
  else if(getMD5(md5, defaultProps, true) && defaultProps == properties)
  {
    myRepository->remove(md5);
    return;
  }

  if(save)
    properties.save(*myRepository->get(md5));
  else
    myTempProps.insert_or_assign(md5, properties);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::loadPerROM(const FSNode& rom, string_view md5)
{
  Properties props;
  bool toInsert = false;

  // First, does this ROM have a per-ROM properties entry?
  // If so, load it into the database
  const FSNode propsNode = rom.getSiblingNode(".pro");
  if(propsNode.exists())
  {
    KeyValueRepositoryPropertyFile repo(propsNode);
    props.load(repo);
    insert(props, false);
  }

  // Next, make sure we have a valid md5 and name
  if(!getMD5(md5, props))
  {
    props.set(PropType::Cart_MD5, md5);
    toInsert = true;
  }
  if(toInsert || props.get(PropType::Cart_Name).empty())
  {
    props.set(PropType::Cart_Name, rom.getBaseName());
    toInsert = true;
  }

  // Finally, insert properties if any info was missing
  if(toInsert)
    insert(props, false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::print() const
{
  // We only look at the temp properties and the built-in ones;
  // any temp entries override the built-in ones.
  // The easiest way to merge the lists is to create a temporary merged map.
  // This isn't fast, but this method is only used for debugging/export.

  // Start with all built-in props
  PropsList list;
  Properties properties;
  for(const auto& DefProp: DefProps)
  {
    properties.setDefaults();
    for(uInt8 p = 0; p < static_cast<uInt8>(PropType::NumTypes); ++p)
      if(DefProp[p][0] != 0)
        properties.set(PropType{p}, DefProp[p]);

    list.emplace(DefProp[static_cast<uInt8>(PropType::Cart_MD5)], properties);
  }

  // Merge temp props, overriding any built-in duplicates
  for(const auto& [md5, props] : myTempProps)
    list.insert_or_assign(md5, props);

  // Print the merged result
  Properties::printHeader();
  for(const auto& [md5, props] : list)
    props.print();
}
