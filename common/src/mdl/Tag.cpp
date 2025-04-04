/*
 Copyright (C) 2010 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Tag.h"

#include "mdl/TagManager.h"

#include "kdl/struct_io.h"

#include <cassert>
#include <ostream>
#include <string>
#include <utility>

namespace tb::mdl
{

Tag::Tag(const size_t index, std::string name, std::vector<TagAttribute> attributes)
  : m_index{index}
  , m_name{std::move(name)}
  , m_attributes{std::move(attributes)}
{
}

Tag::Tag(std::string name, std::vector<TagAttribute> attributes)
  : Tag{0, std::move(name), std::move(attributes)}
{
}

Tag::~Tag() = default;

Tag::Tag(const Tag& other) = default;
Tag::Tag(Tag&& other) noexcept = default;

Tag& Tag::operator=(const Tag& other) = default;
Tag& Tag::operator=(Tag&& other) = default;

TagType::Type Tag::type() const
{
  return TagType::Type(1) << m_index;
}

size_t Tag::index() const
{
  return m_index;
}

void Tag::setIndex(const size_t index)
{
  m_index = index;
}

const std::string& Tag::name() const
{
  return m_name;
}

const std::vector<TagAttribute>& Tag::attributes() const
{
  return m_attributes;
}

std::weak_ordering Tag::operator<=>(const Tag& other) const
{
  return m_name <=> other.m_name;
}

bool Tag::operator==(const Tag& other) const
{
  return m_name == other.m_name;
}

void Tag::appendToStream(std::ostream& str) const
{
  kdl::struct_stream{str} << "Tag"
                          << "m_index" << m_index << "m_name" << m_name << "m_attributes"
                          << m_attributes;
}

std::ostream& operator<<(std::ostream& str, const Tag& tag)
{
  tag.appendToStream(str);
  return str;
}

TagReference::TagReference(const Tag& tag)
  : m_tag{&tag}
{
}

const Tag& TagReference::tag() const
{
  return *m_tag;
}

std::weak_ordering TagReference::operator<=>(const TagReference& other) const
{
  return *m_tag <=> *other.m_tag;
}

bool TagReference::operator==(const TagReference& other) const
{
  return *m_tag == *other.m_tag;
}

Taggable::Taggable() = default;

void swap(Taggable& lhs, Taggable& rhs) noexcept
{
  using std::swap;
  swap(lhs.m_tagMask, rhs.m_tagMask);
  swap(lhs.m_tags, rhs.m_tags);
  swap(lhs.m_attributeMask, rhs.m_attributeMask);
}

Taggable::~Taggable() = default;

bool Taggable::hasAnyTag() const
{
  return m_tagMask != 0;
}

bool Taggable::hasTag(const Tag& tag) const
{
  return hasTag(tag.type());
}

bool Taggable::hasTag(TagType::Type mask) const
{
  return (m_tagMask & mask) != 0;
}

TagType::Type Taggable::tagMask() const
{
  return m_tagMask;
}

bool Taggable::addTag(const Tag& tag)
{
  if (!hasTag(tag))
  {
    m_tagMask |= tag.type();
    m_tags.emplace(tag);

    updateAttributeMask();
    return true;
  }
  return false;
}

bool Taggable::removeTag(const Tag& tag)
{
  if (const auto it = m_tags.find(TagReference(tag)); it != std::end(m_tags))
  {
    m_tagMask &= ~tag.type();
    m_tags.erase(it);
    assert(!hasTag(tag));

    updateAttributeMask();
    return true;
  }

  return false;
}

void Taggable::initializeTags(TagManager& tagManager)
{
  clearTags();
  updateTags(tagManager);
}

void Taggable::updateTags(TagManager& tagManager)
{
  tagManager.updateTags(*this);
  updateAttributeMask();
}

void Taggable::clearTags()
{
  m_tagMask = 0;
  m_tags.clear();
  updateAttributeMask();
}

bool Taggable::hasAttribute(const TagAttribute& attribute) const
{
  return (m_attributeMask & attribute.type) != 0;
}

void Taggable::accept(TagVisitor& visitor)
{
  doAcceptTagVisitor(visitor);
}

void Taggable::accept(ConstTagVisitor& visitor) const
{
  doAcceptTagVisitor(visitor);
}

void Taggable::updateAttributeMask()
{
  m_attributeMask = 0;
  for (const auto& tagRef : m_tags)
  {
    const auto& tag = tagRef.tag();
    for (const auto& attribute : tag.attributes())
    {
      m_attributeMask |= attribute.type;
    }
  }
}

TagMatcherCallback::~TagMatcherCallback() = default;

TagMatcher::~TagMatcher() = default;

void TagMatcher::enable(TagMatcherCallback& /* callback */, MapFacade& /* facade */) const
{
}

void TagMatcher::disable(
  TagMatcherCallback& /* callback */, MapFacade& /* facade */) const
{
}

bool TagMatcher::canEnable() const
{
  return false;
}

bool TagMatcher::canDisable() const
{
  return false;
}

std::ostream& operator<<(std::ostream& str, const TagMatcher& matcher)
{
  matcher.appendToStream(str);
  return str;
}

SmartTag::SmartTag(
  std::string name,
  std::vector<TagAttribute> attributes,
  std::unique_ptr<TagMatcher> matcher)
  : Tag{std::move(name), std::move(attributes)}
  , m_matcher{std::move(matcher)}
{
}

SmartTag::SmartTag(const SmartTag& other)
  : Tag{other.m_index, other.m_name, other.m_attributes}
  , m_matcher{other.m_matcher->clone()}
{
}

SmartTag::SmartTag(SmartTag&& other) noexcept = default;

SmartTag& SmartTag::operator=(const SmartTag& other)
{
  *this = SmartTag{other};
  return *this;
}

SmartTag& SmartTag::operator=(SmartTag&& other) = default;

bool SmartTag::matches(const Taggable& taggable) const
{
  return m_matcher->matches(taggable);
}

void SmartTag::update(Taggable& taggable) const
{
  if (matches(taggable))
  {
    taggable.addTag(*this);
  }
  else
  {
    taggable.removeTag(*this);
  }
}

void SmartTag::enable(TagMatcherCallback& callback, MapFacade& facade) const
{
  m_matcher->enable(callback, facade);
}

void SmartTag::disable(TagMatcherCallback& callback, MapFacade& facade) const
{
  m_matcher->disable(callback, facade);
}

bool SmartTag::canEnable() const
{
  return m_matcher->canEnable();
}

bool SmartTag::canDisable() const
{
  return m_matcher->canDisable();
}

void SmartTag::appendToStream(std::ostream& str) const
{
  kdl::struct_stream{str} << "SmartTag"
                          << "m_index" << m_index << "m_name" << m_name << "m_attributes"
                          << m_attributes << "m_matcher" << *m_matcher;
}

} // namespace tb::mdl
