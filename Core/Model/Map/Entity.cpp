/*
 Copyright (C) 2010-2012 Kristian Duske
 
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
 along with TrenchBroom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Entity.h"
#include <math.h>
#include <iostream>
#include <sstream>

using namespace std;

namespace TrenchBroom {
    namespace Model {
        void Entity::init() {
            m_entityDefinition = NULL;
            m_map = NULL;
            m_filePosition = -1;
            m_selected = false;
            m_vboBlock = NULL;
            m_origin = Null3f;
            m_angle = NAN;
            rebuildGeometry();
        }
        
        void Entity::rebuildGeometry() {
            m_bounds.min = m_bounds.max = Null3f;
            m_maxBounds.min = m_maxBounds.max = Null3f;
            if (m_entityDefinition == NULL || m_entityDefinition->type == EDT_BRUSH) {
                if (!m_brushes.empty()) {
                    m_bounds = m_brushes[0]->bounds();
                    for (int i = 1; i < m_brushes.size(); i++)
                        m_bounds += m_brushes[i]->bounds();
                }
            } else if (m_entityDefinition->type == EDT_POINT) {
                m_bounds = m_entityDefinition->bounds.translate(m_origin);
            }
            
            m_center = m_bounds.center();
            m_maxBounds = m_bounds.maxBounds();
        }
        
        Entity::Entity() : MapObject() {
            init();
        }
        
        Entity::Entity(const map<string, string> properties) : MapObject() {
            init();
            m_properties = properties;
            map<string, string>::iterator it;
            if ((it = m_properties.find(AngleKey)) != m_properties.end())
                m_angle = atof(it->second.c_str());
            if ((it = m_properties.find(OriginKey)) != m_properties.end())
                m_origin = Vec3f(it->second);
            rebuildGeometry();
        }
        
        Entity::~Entity() {
            while (!m_brushes.empty()) delete m_brushes.back(), m_brushes.pop_back();
            
        }
        
        void Entity::rotate90(EAxis axis, Vec3f rotationCenter, bool clockwise) {
            if (m_entityDefinition != NULL && m_entityDefinition->type != EDT_BRUSH)
                return;
            
            setProperty(OriginKey, m_origin.rotate90(axis, rotationCenter, clockwise), true);

            Vec3f direction;
            if (m_angle >= 0) {
                direction.x = cos(2 * M_PI - m_angle * M_PI / 180);
                direction.y = sin(2 * M_PI - m_angle * M_PI / 180);
                direction.z = 0;
            } else if (m_angle == -1) {
                direction = ZAxisPos;
            } else if (m_angle == -2) {
                direction = ZAxisNeg;
            } else {
                return;
            }
            
            direction = direction.rotate90(axis, clockwise);
            if (direction.z > 0.9) {
                setProperty(AngleKey, -1, true);
            } else if (direction.z < -0.9) {
                setProperty(AngleKey, -2, true);
            } else {
                if (direction.z != 0) {
                    direction.z = 0;
                    direction = direction.normalize();
                }
                
                m_angle = roundf(acos(direction.x) * 180 / M_PI);
                Vec3f cross = direction % XAxisPos;
                if (!cross.equals(Null3f) && cross.z < 0)
                    m_angle = 360 - m_angle;
                setProperty(AngleKey, m_angle, true);
            }
        }
        
        const EntityDefinition* Entity::entityDefinition() const {
            return m_entityDefinition;
        }
        
        void Entity::setEntityDefinition(EntityDefinition* entityDefinition) {
            if (m_entityDefinition != NULL)
                m_entityDefinition->usageCount--;
            m_entityDefinition = entityDefinition;
            if (m_entityDefinition != NULL)
                m_entityDefinition->usageCount++;
            rebuildGeometry();
        }
        
        Vec3f Entity::center() const {
            return m_center;
        }
        
        Vec3f Entity::origin() const {
            return m_origin;
        }
        
        BBox Entity::bounds() const {
            return m_bounds;
        }
        
        BBox Entity::maxBounds() const {
            return m_maxBounds;
        }
        
        Map* Entity::quakeMap() const {
            return m_map;
        }
        
        void Entity::setMap(Map* quakeMap) {
            m_map = quakeMap;
        }
        
        const vector<Brush*>& Entity::brushes() const {
            return m_brushes;
        }
        
        const map<string, string> Entity::properties() const {
            return m_properties;
        }
        
        const string* Entity::propertyForKey(const string& key) const {
            map<string, string>::const_iterator it = m_properties.find(key);
            if (it == m_properties.end())
                return NULL;
            return &it->second;
            
        }
        
        bool Entity::propertyWritable(const string& key) const {
            return ClassnameKey != key;
        }
        
        bool Entity::propertyDeletable(const string& key) const {
            if (ClassnameKey == key)
                return false;
            if (OriginKey == key)
                return false;
            if (SpawnFlagsKey == key)
                return false;
            return true;
        }
        
        void Entity::setProperty(const string& key, const string& value) {
            setProperty(key, &value);
        }
        
        void Entity::setProperty(const string& key, const string* value) {
            if (key == ClassnameKey && classname() != NULL) {
                fprintf(stdout, "Warning: Cannot overwrite classname property");
                return;
            } else if (key == OriginKey) {
                if (value == NULL) {
                    fprintf(stdout, "Warning: Cannot set origin to NULL");
                    return;
                }
                m_origin = Vec3f(*value);
            } else if (key == AngleKey) {
                if (value != NULL) m_angle = strtof(value->c_str(), NULL);
                else m_angle = NAN;
            }
            
            const string* oldValue = propertyForKey(key);
            if (oldValue != NULL && oldValue == value) return;
            m_properties[key] = *value;
            rebuildGeometry();
        }
        
        void Entity::setProperty(const string& key, Vec3f value, bool round) {
            stringstream valueStr;
            if (round) valueStr << (int)roundf(value.x) << " " << (int)roundf(value.y) << " " << (int)roundf(value.z);
            else valueStr << value.x << " " << value.y << " " << value.z;
            setProperty(key, valueStr.str());
        }
        
        void Entity::setProperty(const string& key, float value, bool round) {
            stringstream valueStr;
            if (round) valueStr << (int)roundf(value);
            else valueStr << value;
            setProperty(key, valueStr.str());
        }
        
        void Entity::setProperties(map<string, string> properties, bool replace) {
            if (replace) m_properties.clear();
            map<string, string>::iterator it;
            for (it = properties.begin(); it != properties.end(); ++it)
                m_properties[it->first] = it->second;
        }
        
        void Entity::deleteProperty(const string& key) {
            if (!propertyDeletable(key)) {
                fprintf(stdout, "Warning: Cannot delete property '%s'", key.c_str());
                return;
            }
            
            if (key == AngleKey) m_angle = NAN;
            if (m_properties.count(key) == 0) return;
            m_properties.erase(key);
            rebuildGeometry();
        }
        
        const string* Entity::classname() const {
            return propertyForKey(ClassnameKey);
        }
        
        const int Entity::angle() const {
            return roundf(m_angle);
        }
        
        bool Entity::worldspawn() const {
            return *classname() == WorldspawnClassname;
        }
        
        bool Entity::group() const {
            return *classname() == GroupClassname;
        }
        
        void Entity::addBrush(Brush* brush) {
            if (m_entityDefinition != NULL && m_entityDefinition->type != EDT_BRUSH)
                return;
            
            brush->setEntity(this);
            m_brushes.push_back(brush);
            rebuildGeometry();
        }
        
        void Entity::addBrushes(const vector<Brush*>& brushes) {
            if (m_entityDefinition != NULL && m_entityDefinition->type != EDT_BRUSH)
                return;
            
            for (int i = 0; i < brushes.size(); i++) {
                brushes[i]->setEntity(this);
                m_brushes.push_back(brushes[i]);
            }
            rebuildGeometry();
        }
        
        void Entity::brushChanged(Brush* brush) {
            rebuildGeometry();
        }
        
        void Entity::removeBrush(Brush* brush) {
            if (m_entityDefinition != NULL && m_entityDefinition->type != EDT_BRUSH)
                return;
            
            brush->setEntity(NULL);
            m_brushes.erase(find(m_brushes.begin(), m_brushes.end(), brush));
            rebuildGeometry();
        }
        
        void Entity::removeBrushes(vector<Brush*>& brushes) {
            if (m_entityDefinition != NULL && m_entityDefinition->type != EDT_BRUSH)
                return;
            
            for (int i = 0; i < brushes.size(); i++) {
                brushes[i]->setEntity(NULL);
                m_brushes.erase(find(m_brushes.begin(), m_brushes.end(), brushes[i]));
            }
            rebuildGeometry();
        }
        
        void Entity::translate(Vec3f delta) {
            if (m_entityDefinition != NULL && m_entityDefinition->type != EDT_POINT)
                return;
            
            setProperty(OriginKey, m_origin + delta, true);
        }
        
        void Entity::rotate90CW(EAxis axis, Vec3f rotationCenter) {
            rotate90(axis, rotationCenter, true);
        }
        
        void Entity::rotate90CCW(EAxis axis, Vec3f rotationCenter) {
            rotate90(axis, rotationCenter, false);
        }
        
        void Entity::rotate(Quat rotation, Vec3f rotationCenter) {
            if (m_entityDefinition != NULL && m_entityDefinition->type != EDT_BRUSH)
                return;
            
            Vec3f offset = m_center - m_origin;
            m_center = rotation * (m_center - rotationCenter) + rotationCenter;
            setProperty(OriginKey, m_center - offset, true);
            setProperty(AngleKey, 0, true);
            
            Vec3f direction;
            if (m_angle >= 0) {
                direction.x = cos(2 * M_PI - m_angle * M_PI / 180);
                direction.y = sin(2 * M_PI - m_angle * M_PI / 180);
                direction.z = 0;
            } else if (m_angle == -1) {
                direction = ZAxisPos;
            } else if (m_angle == -2) {
                direction = ZAxisNeg;
            } else {
                return;
            }
            
            direction = rotation * direction;
            if (direction.z > 0.9) {
                setProperty(AngleKey, -1, true);
            } else if (direction.z < -0.9) {
                setProperty(AngleKey, -2, true);
            } else {
                if (direction.z != 0) {
                    direction.z = 0;
                    direction = direction.normalize();
                }
                
                m_angle = roundf(acos(direction.x) * 180 / M_PI);
                Vec3f cross = direction % XAxisPos;
                if (!cross.equals(Null3f) && cross.z < 0)
                    m_angle = 360 - m_angle;
                setProperty(AngleKey, m_angle, true);
            }
        }
        
        void Entity::flip(EAxis axis, Vec3f flipCenter) {
            if (m_entityDefinition != NULL && m_entityDefinition->type != EDT_BRUSH)
                return;
            
            Vec3f offset = m_center - m_origin;
            m_center = m_center.flip(axis, flipCenter);
            setProperty(OriginKey, m_center + offset, true);
            setProperty(AngleKey, 0, true);
            
            if (m_angle >= 0)
                m_angle = (m_angle + 180) - (int)((m_angle / 360)) * m_angle;
            else if (m_angle == -1)
                m_angle = -2;
            else if (m_angle == -2)
                m_angle = -1;
            setProperty(AngleKey, m_angle, true);
        }
        
        int Entity::filePosition() const {
            return m_filePosition;
        }
        
        void Entity::setFilePosition(int filePosition) {
            m_filePosition = filePosition;
        }
        
        bool Entity::selected() const {
            return m_selected;
        }
        
        void Entity::setSelected(bool selected) {
            m_selected = selected;
        }
        
        Renderer::VboBlock* Entity::vboBlock() const {
            return m_vboBlock;
        }
        
        void Entity::setVboBlock(Renderer::VboBlock* vboBlock) {
            if (m_vboBlock != NULL)
                vboBlock->freeBlock();
            m_vboBlock = vboBlock;
        }
    }
}
