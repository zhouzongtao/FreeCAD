/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <bitset>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <QIcon>
#include <fastsignals/signal.h>
#include <boost/intrusive_ptr.hpp>

#include <App/Material.h>
#include <App/TransactionalObject.h>
#include <Base/BoundBox.h>
#include <Base/Vector3D.h>

#include "TreeItemMode.h"

class SbVec2s;
class SbVec3f;
class SoNode;
class SoPath;
class SoSeparator;
class SoEvent;
class SoSwitch;
class SoTransform;
class SbMatrix;
class SoEventCallback;
class SoPickedPoint;
class SoDetail;
class SoFullPath;
class QString;
class QMenu;
class QObject;


namespace Base
{
class Matrix4D;
class Color;
}  // namespace Base

class SoGroup;

// 注意：移除 "class RenderNode;" 前向声明以避免与 Gui::Render::RenderNode 冲突
// Note: Remove "class RenderNode;" forward declaration to avoid conflict with Gui::Render::RenderNode

// 前向声明渲染抽象层 / Forward declarations for rendering abstraction layer
namespace Gui {
namespace Render {
    class RenderNode;
    class RenderGroup;
    class RenderSeparator;
    class RenderNodeFactory;
    enum class BackendType : uint8_t;
}
}

namespace Gui
{
namespace TaskView
{
class TaskContent;
}
class View3DInventorViewer;
class ViewProviderPy;
class ObjectItem;
class MDIView;
class SelectionChanges;

enum ViewStatus
{
    UpdateData = 0,
    Detach = 1,
    isRestoring = 2,
    UpdatingView = 3,
    TouchDocument = 4,
};


/** Convenience smart pointer to wrap coin node.
 *
 * It is basically boost::intrusive plus implicit pointer conversion to save the
 * trouble of typing get() all the time.
 */
template<class T>
class CoinPtr: public boost::intrusive_ptr<T>
{
public:
    // Too bad, VC2013 does not support constructor inheritance
    // using boost::intrusive_ptr<T>::intrusive_ptr;
    using inherited = boost::intrusive_ptr<T>;
    CoinPtr() = default;
    CoinPtr(T* p, bool add_ref = true)
        : inherited(p, add_ref)
    {}
    template<class Y>
    CoinPtr(CoinPtr<Y> const& r)
        : inherited(r)
    {}

    operator T*() const
    {
        return this->get();
    }  // explicit bombs
};

/** Helper function to deal with bug in SoNode::removeAllChildren()
 *
 * @sa https://bitbucket.org/Coin3D/coin/pull-requests/119/fix-sochildlist-auditing/diff
 */
void GuiExport coinRemoveAllChildren(SoGroup* node);

/** General interface for all visual stuff in FreeCAD
 * This class is used to generate and handle all around
 * visualizing and presenting objects from the FreeCAD
 * App layer to the user. This class and its descendents
 * have to be implemented for any object type in order to
 * show them in the 3DView and TreeView.
 */
class GuiExport ViewProvider: public App::TransactionalObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProvider);

public:
    enum class ToggleVisibilityMode : bool
    {
        CanToggleVisibility = true,
        NoToggleVisibility = false
    };

    /// constructor.
    ViewProvider();

    /// destructor.
    ~ViewProvider() override;

    // returns the root node of the Provider (3D)
    virtual SoSeparator* getRoot() const
    {
        return pcRoot;
    }

    // returns the abstract render root node (for rendering abstraction layer)
    // 返回抽象层根节点，用于后端无关的渲染
    // Returns the abstraction layer root node for backend-agnostic rendering
    virtual Render::RenderNode* getRenderRoot() const;

    /**
     * @brief 获取抽象层根节点（智能指针版本）
     * Get abstraction layer root node (smart pointer version)
     * 返回 RenderNode，可 dynamic_cast 到 RenderSeparator 或具体后端类型
     * Returns RenderNode, use dynamic_cast for RenderSeparator or backend-specific types
     */
    std::shared_ptr<Render::RenderNode> getRenderRootPtr() const;

    /**
     * @brief 获取抽象层模式切换节点
     * Get abstraction layer mode switch node
     */
    std::shared_ptr<Render::RenderNode> getRenderModeSwitch() const;

    /**
     * @brief 获取抽象层变换节点
     * Get abstraction layer transform node
     */
    std::shared_ptr<Render::RenderNode> getRenderTransform() const;

    /**
     * @brief 获取当前使用的后端类型
     * Get the currently used backend type
     */
    virtual Render::BackendType getBackendType() const;

    /**
     * @brief 获取节点工厂
     * Get node factory for creating backend-specific nodes
     */
    Render::RenderNodeFactory* getNodeFactory() const;

    // return the mode switch node of the Provider (3D)
    SoSwitch* getModeSwitch() const
    {
        return pcModeSwitch;
    }
    SoTransform* getTransformNode() const
    {
        return pcTransform;
    }
    // returns the root for the Annotations.
    SoSeparator* getAnnotation();
    // returns the root node of the Provider (3D)
    virtual SoSeparator* getFrontRoot() const;
    // returns the root node where the children gets collected(3D)
    virtual SoGroup* getChildRoot() const;
    // returns the root node of the Provider (3D)
    virtual SoSeparator* getBackRoot() const;

    /**
     * @brief 获取前景层根节点（抽象层）
     * Get front root node (abstraction layer)
     *
     * 用于前景层渲染，如标注、选择高亮等
     * For front layer rendering, such as annotations, selection highlights
     *
     * 注意：返回 RenderNode 以避免继承问题，可通过 dynamic_cast 转换
     * Note: Returns RenderNode to avoid inheritance issues, use dynamic_cast
     */
    virtual std::shared_ptr<Render::RenderNode> getRenderFrontRoot() const;

    /**
     * @brief 获取子对象根节点（抽象层）
     * Get child root node (abstraction layer)
     *
     * 用于收集子对象的渲染节点
     * For collecting child objects' render nodes
     *
     * 注意：返回 RenderNode 以避免继承问题，可通过 dynamic_cast 转换
     * Note: Returns RenderNode to avoid inheritance issues, use dynamic_cast
     */
    virtual std::shared_ptr<Render::RenderNode> getRenderChildRoot() const;

    /**
     * @brief 获取背景层根节点（抽象层）
     * Get back root node (abstraction layer)
     *
     * 用于背景层渲染
     * For back layer rendering
     *
     * 注意：返回 RenderNode 以避免继承问题，可通过 dynamic_cast 转换
     * Note: Returns RenderNode to avoid inheritance issues, use dynamic_cast
     */
    virtual std::shared_ptr<Render::RenderNode> getRenderBackRoot() const;
    /// Indicate whether to be added to scene graph or not
    virtual bool canAddToSceneGraph() const
    {
        return true;
    }
    // Indicate whether to be added to object group (true) or only to scene graph (false)
    virtual bool isPartOfPhysicalObject() const
    {
        return true;
    }

    /** deliver the children belonging to this object
     * this method is used to deliver the objects to
     * the 3DView which should be grouped under its
     * scene graph. This affects the visibility and the 3D
     * position of the object.
     */
    virtual std::vector<App::DocumentObject*> claimChildren3D() const;

    /** @name Selection handling
     * This group of methods do the selection handling.
     * Here you can define how the selection for your ViewProfider
     * works.
     */
    //@{

    /// indicates if the ViewProvider use the new Selection model
    virtual bool useNewSelectionModel() const;
    virtual bool isSelectable() const
    {
        return true;
    }
    /// called when the selection changes for the view provider
    virtual void onSelectionChanged(const SelectionChanges&)
    {}
    /// return a hit element given the picked point which contains the full node path
    virtual bool getElementPicked(const SoPickedPoint*, std::string& subname) const;
    /// return a hit element to the selection path or 0
    virtual std::string getElement(const SoDetail*) const
    {
        return {};
    }
    /// return the coin node detail of the subelement
    virtual SoDetail* getDetail(const char*) const
    {
        return nullptr;
    }

    /** return the coin node detail and path to the node of the subelement
     *
     * @param subname: dot separated string reference to the sub element
     * @param pPath: output coin path leading to the returned element detail
     * @param append: If true, pPath will be first appended with the root node and
     * the mode switch node of this view provider.
     *
     * @return the coin detail of the subelement
     *
     * If this view provider links to other view provider, then the
     * implementation of getDetailPath() shall also append all intermediate
     * nodes starting just after the mode switch node up till the mode switch of
     * the linked view provider.
     */
    virtual bool getDetailPath(const char* subname, SoFullPath* pPath, bool append, SoDetail*& det) const;

    /** partial rendering setup
     *
     * @param subelements: a list of dot separated string refer to the sub element
     * @param clear: if true, remove the subelement from partial rendering.
     * If else, add the subelement for rendering.
     *
     * @return Return the number of subelement found
     *
     * Partial rendering only works if there is at least one SoFCSelectRoot node
     * in this view provider
     */
    int partialRender(const std::vector<std::string>& subelements, bool clear);

    virtual std::vector<Base::Vector3d> getModelPoints(const SoPickedPoint*) const;
    /// return the highlight lines for a given element or the whole shape
    virtual std::vector<Base::Vector3d> getSelectionShape(const char* Element) const
    {
        (void)Element;
        return {};
    }

    /** Return the bound box of this view object
     *
     * This method shall work regardless whether the current view object is
     * visible or not.
     */
    Base::BoundBox3d getBoundingBox(
        const char* subname = nullptr,
        bool transform = true,
        MDIView* view = nullptr
    ) const;

    /**
     * Get called if the object is about to get deleted.
     * Here you can delete other objects, switch their visibility or prevent the deletion of the
     * object.
     * @param subNames  list of selected subelements
     * @return          true if the deletion is approved by the view provider.
     */
    virtual bool onDelete(const std::vector<std::string>& subNames);
    /** Called before deletion
     *
     * Unlike onDelete(), this function is guaranteed to be called before
     * deletion, either by Document::remObject(), or on document deletion.
     */
    virtual void beforeDelete();
    /**
     * @brief Asks the view provider if the given object that is part of its
     * outlist can be removed from there without breaking it.
     * @param obj is part of the outlist of the object associated to the view provider
     * @return true if the removal is approved by the view provider.
     */
    virtual bool canDelete(App::DocumentObject* obj) const;
    //@}


    /** @name Methods used by the Tree
     * If you want to take control over the
     * appearance of your object in the tree you
     * can reimplement these methods.
     */
    //@{
    /// deliver the icon shown in the tree view
    virtual QIcon getIcon() const;

    /**
     * @brief Whether the viewprovider should allow to toggle the visibility.
     *
     * Some document objects are not rendered and for those document objects,
     * it makes no sense to be able to toggle the visibility.  Examples are
     * VarSet and Spreadsheet.
     *
     * Note that "rendered" should be seen broadly here.  Objects such as
     * TechDraw pages, templates, views, and dimensions are not rendered by
     * Coin but are "rendered" on the TechDraw page and hence this function can
     * return true for those items.
     */
    bool canToggleVisibility() const
    {
        return toggleVisibilityMode == ToggleVisibilityMode::CanToggleVisibility;
    }

    /** @name Methods used by the Tree
     * If you want to take control over the
     * viewprovider specific overlay icons that will be drawn with color
     * regardless of whether the icon is greyed out or not, such as status, you
     * can reimplement this method.
     */
    virtual QIcon mergeColorfulOverlayIcons(const QIcon& orig) const;

    /** deliver the children belonging to this object
     * this method is used to deliver the objects to
     * the tree framework which should be grouped under its
     * label. Obvious is the usage in the group but it can
     * be used for any kind of grouping needed for a special
     * purpose.
     */
    virtual std::vector<App::DocumentObject*> claimChildren() const;
    //@}

    /** deliver the children belonging to this object recursively.
     */
    virtual std::vector<App::DocumentObject*> claimChildrenRecursive() const;
    //@}

    /** @name Drag and drop
     * To enable drag and drop you have to re-implement \ref canDragObjects() and
     * \ref canDropObjects() to return true. For finer control you can also re-implement
     * \ref canDragObject() or \ref canDropObject() to filter certain object types, by
     * default these methods don't filter any types.
     * To take action of drag and drop the method \ref dragObject() and \ref dropObject()
     * must be re-implemented, too.
     */
    //@{
    /** Check whether children can be removed from the view provider by drag and drop */
    virtual bool canDragObjects() const;
    /** Check whether the object can be removed from the view provider by drag and drop */
    virtual bool canDragObject(App::DocumentObject*) const;
    /** Check whether the object can be removed from the view provider by drag and drop to a
     * determined target*/
    virtual bool canDragObjectToTarget(App::DocumentObject* obj, App::DocumentObject* target) const;
    /** Remove a child from the view provider by drag and drop */
    virtual void dragObject(App::DocumentObject*);
    /** Check whether objects can be added to the view provider by drag and drop or drop only */
    virtual bool canDropObjects() const;
    /** Check whether the object can be dropped to the view provider by drag and drop or drop only*/
    virtual bool canDropObject(App::DocumentObject*) const;
    /** Return false to force drop only operation for a given object*/
    virtual bool canDragAndDropObject(App::DocumentObject*) const;
    /** Add an object to the view provider by drag and drop */
    virtual void dropObject(App::DocumentObject*);
    /** Query object dropping with full qualified name
     *
     * Tree view now calls this function instead of canDropObject(), and may
     * query for objects from other document. The default implementation
     * (actually in ViewProviderDocumentObject) inhibites cross document
     * dropping, and calls canDropObject(obj) for the rest. Override this
     * function to enable cross document linking.
     *
     * @param obj: the object being dropped
     *
     * @param owner: the (grand)parent object of the dropping object. Maybe
     * null. This may not be the top parent object, as tree view will try to
     * find a parent of the dropping object relative to this object to avoid
     * cyclic dependency
     *
     * @param subname: subname reference to the dropping object
     *
     * @param elements: non-object sub-elements, e.g. Faces, Edges, selected
     * when the object is being dropped
     *
     * @return Return whether the dropping action is allowed.
     * */
    virtual bool canDropObjectEx(
        App::DocumentObject* obj,
        App::DocumentObject* owner,
        const char* subname,
        const std::vector<std::string>& elements
    ) const;
    /* Check whether the object accept reordering of its children during drop.*/
    virtual bool acceptReorderingObjects() const
    {
        return false;
    };

    /// return a subname referencing the sub-object holding the dropped objects
    virtual std::string getDropPrefix() const
    {
        return {};
    }

    /** Add an object with full qualified name to the view provider by drag and drop
     *
     * @param obj: the object being dropped
     *
     * @param owner: the (grand)parent object of the dropping object. Maybe
     * null. This may not be the top parent object, as tree view will try to
     * find a parent of the dropping object relative to this object to avoid
     * cyclic dependency
     *
     * @param subname: subname reference to the dropping object
     *
     * @param elements: non-object sub-elements, e.g. Faces, Edges, selected
     * when the object is being dropped
     *
     * @return Optionally returns a subname reference locating the dropped
     * object, which may or may not be the actual dropped object, e.g. it may be
     * a link.
     */
    virtual std::string dropObjectEx(
        App::DocumentObject* obj,
        App::DocumentObject* owner,
        const char* subname,
        const std::vector<std::string>& elements
    );
    /** Replace an object to the view provider by drag and drop
     *
     * @param oldObj: object to be replaced
     * @param newObj: object to replace with
     *
     * @return Returns 0 if not found, 1 if succeeded, -1 if not supported
     */
    virtual int replaceObject(App::DocumentObject* oldObj, App::DocumentObject* newObj);
    //@}

    /** Tell the tree view if this object should appear there */
    virtual bool showInTree() const
    {
        return true;
    }
    /** Tell the tree view to remove children items from the tree root*/
    virtual bool canRemoveChildrenFromRoot() const
    {
        return true;
    }
    /** Tell if the tree item should be auto collapsed*/
    bool isAutoCollapseOnDeactivation() const
    {
        return autoCollapseOnDeactivation;
    }

    /** @name Signals of the view provider */
    //@{
    /// signal on icon change
    fastsignals::signal<void()> signalChangeIcon;
    /// signal on tooltip change
    fastsignals::signal<void(const QString&)> signalChangeToolTip;
    /// signal on status tip change
    fastsignals::signal<void(const QString&)> signalChangeStatusTip;
    /// signal on highlight change
    fastsignals::signal<void(bool, Gui::HighlightMode)> signalChangeHighlight;
    //@}

    /** update the content of the ViewProvider
     * this method have to implement the recalculation
     * of the ViewProvider. There are different reasons to
     * update. E.g. only the view attribute has changed, or
     * the data has manipulated.
     */
    virtual void update(const App::Property*);
    virtual void updateData(const App::Property*);
    bool isUpdatesEnabled() const;
    void setUpdatesEnabled(bool enable);

    /// return the status bits
    unsigned long getStatus() const
    {
        return StatusBits.to_ulong();
    }
    bool testStatus(ViewStatus pos) const
    {
        return StatusBits.test((size_t)pos);
    }
    void setStatus(ViewStatus pos, bool on)
    {
        StatusBits.set((size_t)pos, on);
    }

    std::string toString() const;
    PyObject* getPyObject() override;

    /** @name Display mode methods
     */
    //@{
    std::string getActiveDisplayMode() const;
    /// set the display mode
    virtual void setDisplayMode(const char* ModeName);
    /// get the default display mode
    virtual const char* getDefaultDisplayMode() const;
    /// returns a list of all possible display modes
    virtual std::vector<std::string> getDisplayModes() const;
    /// Hides the view provider
    virtual void hide();
    /// Shows the view provider
    virtual void show();
    /// checks whether the view provider is visible or not
    virtual bool isShow() const;
    void setVisible(bool);
    bool isVisible() const;
    void setLinkVisible(bool);
    bool isLinkVisible() const;
    /// Overrides the display mode with mode.
    virtual void setOverrideMode(const std::string& mode);
    const std::string getOverrideMode();
    //@}

    /** @name Color management methods
     */
    //@{
    virtual std::map<std::string, Base::Color> getElementColors(const char* element = nullptr) const
    {
        (void)element;
        return {};
    }
    virtual void setElementColors(const std::map<std::string, Base::Color>& colors)
    {
        (void)colors;
    }
    static const std::string& hiddenMarker();
    static const char* hasHiddenMarker(const char* subname);
    //@}

    /** @name Edit methods
     * if the Viewprovider goes in edit mode
     * you can handle most of the events in the viewer by yourself
     */
    //@{
    // the below enum is reflected in 'userEditModes' std::map in Application.h
    // so it is possible for the user to choose a default one through GUI
    // if you add a mode here, consider to make it accessible there too
    enum EditMode
    {
        Default = 0,
        Transform,
        Cutting,
        Color,
    };

protected:
    /// is called by the document when the provider goes in edit mode
    virtual bool setEdit(int ModNum);
    /// is called when you lose the edit mode
    virtual void unsetEdit(int ModNum);
    /// return the edit mode or -1 if nothing is being edited
    int getEditingMode() const;

public:
    virtual ViewProvider* startEditing(int ModNum = 0);
    bool isEditing() const;
    void finishEditing();
    /// adjust viewer settings when editing a view provider
    virtual void setEditViewer(View3DInventorViewer*, int ModNum);
    /// restores viewer settings when leaving editing mode
    virtual void unsetEditViewer(View3DInventorViewer*);
    //@}

    /** @name Task panel
     * With this interface the ViewProvider can steer the
     * appearance of widgets in the task view
     */
    //@{
    /// get a list of TaskBoxes associated with this object
    virtual void getTaskViewContent(std::vector<Gui::TaskView::TaskContent*>&) const
    {}
    //@}

    /// is called when the provider is in edit and a "Select All" command was issued
    /// Provider shall return 'false' is it ignores the command, 'true' otherwise
    virtual bool selectAll()
    {
        return false;
    }
    /// is called when the provider is in edit and a key event occurs. Only ESC ends edit.
    virtual bool keyPressed(bool pressed, int key);
    /// Is called by the tree if the user double clicks on the object. It returns the string
    /// for the transaction that will be shown in the undo/redo dialog.
    /// If null is returned then no transaction will be opened.
    virtual const char* getTransactionText() const
    {
        return nullptr;
    }
    /// is called by the tree if the user double clicks on the object
    virtual bool doubleClicked()
    {
        return false;
    }
    /// is called when the provider is in edit and the mouse is moved
    virtual bool mouseMove(const SbVec2s& cursorPos, View3DInventorViewer* viewer);
    /// is called when the Provider is in edit and the mouse is clicked
    virtual bool mouseButtonPressed(
        int button,
        bool pressed,
        const SbVec2s& cursorPos,
        const View3DInventorViewer* viewer
    );

    virtual bool mouseWheelEvent(int delta, const SbVec2s& cursorPos, const View3DInventorViewer* viewer);
    /// set up the context-menu with the supported edit modes
    virtual void setupContextMenu(QMenu*, QObject*, const char*);

    /** @name direct handling methods
     *  This group of methods is to direct influence the
     *  appearance of the viewed content. It's only for fast
     *  interactions! If you want to set the visual parameters
     *  you have to do it on the object viewed by this provider!
     */
    //@{
    /// set the viewing transformation of the provider
    virtual void setTransformation(const Base::Matrix4D& rcMatrix);
    virtual void setTransformation(const SbMatrix& rcMatrix);
    static SbMatrix convert(const Base::Matrix4D& rcMatrix);
    static Base::Matrix4D convert(const SbMatrix& sbMat);
    //@}

    virtual MDIView* getMDIView() const
    {
        return nullptr;
    }

public:
    // this method is called by the viewer when the ViewProvider is in edit
    static void eventCallback(void* ud, SoEventCallback* node);

    // restoring the object from document:
    // this may be of interest to extensions, hence call them
    void Restore(Base::XMLReader& reader) override;
    bool isRestoring()
    {
        return testStatus(Gui::isRestoring);
    }


    /** @name Display mask modes
     * Mainly controls an SoSwitch node which selects the display mask modes.
     * The number of display mask modes doesn't necessarily match with the number
     * of display modes.
     * E.g. various display modes like Gaussian curvature, mean curvature or gray
     * values are displayed by one display mask mode that handles color values.
     */
    //@{
    /// Adds a new display mask mode (Coin3D node)
    /// @deprecated Use addDisplayMaskMode(RenderNode::Ptr, const char*) for new code
    void addDisplayMaskMode(SoNode* node, const char* type);

    /**
     * @brief 添加显示模式（抽象层节点）
     * Add display mask mode (abstraction layer node)
     *
     * @param node 抽象层节点 / Abstraction layer node
     * @param type 模式名称 / Mode name
     */
    void addDisplayMaskMode(std::shared_ptr<Render::RenderNode> node, const char* type);

    /// Activates the display mask mode \a type
    void setDisplayMaskMode(const char* type);
    /// Get the node to the display mask mode \a type (Coin3D)
    SoNode* getDisplayMaskMode(const char* type) const;

    /**
     * @brief 获取显示模式节点（抽象层）
     * Get display mask mode node (abstraction layer)
     */
    std::shared_ptr<Render::RenderNode> getRenderDisplayMaskMode(const char* type) const;

    /// Returns a list of added display mask modes
    std::vector<std::string> getDisplayMaskModes() const;
    void setDefaultMode(int);
    int getDefaultMode() const;
    //@}

    virtual void setRenderCacheMode(int);

protected:
    /** Helper method to check that the node is valid, i.e. it must not cause
     * and infinite recursion.
     */
    bool checkRecursion(SoNode*);
    /** Helper method to get picked entities while editing.
     * It's in the responsibility of the caller to delete the returned instance.
     */
    SoPickedPoint* getPointOnRay(const SbVec2s& pos, const View3DInventorViewer* viewer) const;
    /** Helper method to get picked entities while editing.
     * It's in the responsibility of the caller to delete the returned instance.
     */
    SoPickedPoint* getPointOnRay(
        const SbVec3f& pos,
        const SbVec3f& dir,
        const View3DInventorViewer* viewer
    ) const;
    /// Reimplemented from subclass
    void onBeforeChange(const App::Property* prop) override;
    /// Reimplemented from subclass
    void onChanged(const App::Property* prop) override;

    /** @name Methods used by the Tree
     * If you want to take control over the
     * viewprovider specific overlay icons, that will be grayed out together
     * with the base icon, you can reimplement this method.
     */
    virtual QIcon mergeGreyableOverlayIcons(const QIcon& orig) const;

    /// Turn on mode switch
    virtual void setModeSwitch();

    void setToggleVisibility(ToggleVisibilityMode mode)
    {
        toggleVisibilityMode = mode;
    }

    /**
     * @brief 初始化抽象层渲染节点
     * Initialize abstraction layer render nodes
     *
     * 通过工厂创建后端无关的节点结构。
     * Creates backend-agnostic node structure through factory.
     */
    virtual void initRenderNodes();

    /**
     * @brief 同步模式切换到抽象层节点
     * Sync mode switch to abstraction layer node
     *
     * @param modeIndex 模式索引，-1 表示隐藏 / Mode index, -1 means hidden
     */
    void syncModeSwitchToRenderNode(int modeIndex);

protected:
    //=========================================================================
    // Coin3D 节点（向后兼容，过渡期保留）
    // Coin3D nodes (for backward compatibility, kept during transition)
    //=========================================================================

    /// The root Separator of the ViewProvider (Coin3D)
    /// @deprecated Use m_renderRoot instead for new code
    SoSeparator* pcRoot;
    /// this is transformation for the provider (Coin3D)
    /// @deprecated Use m_renderTransform instead for new code
    SoTransform* pcTransform;
    const char* sPixmap;
    /// this is the mode switch, all the different viewing modes are collected here (Coin3D)
    /// @deprecated Use m_renderModeSwitch instead for new code
    SoSwitch* pcModeSwitch;
    /// The root separator for annotations (Coin3D)
    SoSeparator* pcAnnotation {nullptr};

    //=========================================================================
    // 抽象层节点（方案B：后端完全平等）
    // Abstraction layer nodes (Plan B: backends are fully equal)
    //=========================================================================

    /// 抽象层根节点 / Abstraction layer root node
    /// 实际类型取决于后端，可 dynamic_cast 为 RenderGroup/RenderSeparator
    /// Actual type depends on backend, can dynamic_cast to RenderGroup/RenderSeparator
    std::shared_ptr<Render::RenderNode> m_renderRoot;
    /// 抽象层变换节点 / Abstraction layer transform node
    std::shared_ptr<Render::RenderNode> m_renderTransform;
    /// 抽象层模式切换节点 / Abstraction layer mode switch node
    std::shared_ptr<Render::RenderNode> m_renderModeSwitch;

    //=========================================================================
    // 其他成员 / Other members
    //=========================================================================

    ViewProviderPy* pyViewObject {nullptr};
    bool autoCollapseOnDeactivation {true};
    std::string overrideMode;
    std::bitset<32> StatusBits;
    /// whether visibility can toggled
    ToggleVisibilityMode toggleVisibilityMode;

    friend class ViewProviderPy;

private:
    int _iActualMode {-1};
    int _iEditMode {-1};
    int viewOverrideMode {-1};
    std::string _sCurrentMode;
    std::map<std::string, int> _sDisplayMaskModes;

    /// 抽象层显示模式节点映射 / Abstraction layer display mode nodes
    std::map<std::string, std::shared_ptr<Render::RenderNode>> m_renderDisplayMaskModes;
};

}  // namespace Gui
