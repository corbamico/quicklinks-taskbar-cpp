#include "pch.h"
using namespace winrt;
using namespace winrt::Windows::Foundation;
using json = nlohmann::json;

namespace ns {
    struct Item
    {
        std::string title;
        std::string path;
        std::string args;
    };

    struct Config
    {
        //std::string category;
        std::vector<Item> items;
        std::string action;
        u_short iconInTaskbar;
    };

    void from_json(const json& j, Item& p)
    {
        j.at("title").get_to(p.title);
        j.at("path").get_to(p.path);
        j.at("args").get_to(p.args);
    }

    void from_json(const json& j, Config& p)
    {
        //j.at("category").get_to(p.category);
        j.at("items").get_to(p.items);
        j.at("action").get_to(p.action);
        j.at("iconInTaskbar").get_to(p.iconInTaskbar);        
    }
}

class Engine
{
public:
    Engine()
    {
        winrt::init_apartment();
    }
    ~Engine() { winrt::uninit_apartment(); }
    HRESULT CreateJumpList(const ns::Config& config)
    {
        auto customDestinationList = wil::CoCreateInstance<ICustomDestinationList, wil::err_returncode_policy>(CLSID_DestinationList);

        UINT maxSlots;
        winrt::com_ptr<IObjectArray> removedItems;
        RETURN_IF_FAILED(customDestinationList->BeginList(&maxSlots, IID_PPV_ARGS(removedItems.put())));

        // Create a custom category
        auto objectCollection = wil::CoCreateInstance<IObjectCollection, wil::err_returncode_policy>(CLSID_EnumerableObjectCollection);

        // Add tasks to the custom category
        for(auto& item: config.items)
        {
            AddTaskToCollection(objectCollection.get(),
                                string_to_w(item.title).c_str(),
                                string_to_w(item.path).c_str(),
                                string_to_w(item.args).c_str()
            );
        }

        auto objectArray = objectCollection.try_query<IObjectArray>();
        auto hr = customDestinationList->AddUserTasks(objectArray.get());
        customDestinationList->CommitList();
        return hr;
    }
    void ShowMenu(u_short nth)
    {
        POINT pt = GetTaskBarPoint(nth);
        POINT ptCur{};

        GetCursorPos(&ptCur);
        ShowCursor(FALSE);
        SetCursorPos(pt.x, pt.y);

        INPUT inputs[2] = {};
        inputs[0].type = INPUT_MOUSE;
        inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        inputs[1].type = INPUT_MOUSE;
        inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;

        SendInput(2, inputs, sizeof(INPUT));
        SetCursorPos(ptCur.x, ptCur.y);
        ShowCursor(TRUE);
    }    
private:
    HRESULT AddTaskToCollection(IObjectCollection *collection, LPCWSTR title, LPCWSTR appPath, LPCWSTR args)
    {
        // use wil::err_returncode_policy to return hresult fast.
        auto shellLink = wil::CoCreateInstance<IShellLink, wil::err_returncode_policy>(CLSID_ShellLink);
        RETURN_IF_FAILED(shellLink->SetPath(appPath));
        RETURN_IF_FAILED(shellLink->SetArguments(args));
        RETURN_IF_FAILED(shellLink->SetDescription(title));
        {
            auto propertyStore = shellLink.try_query<IPropertyStore>();
            wil::unique_prop_variant propVariant;
            RETURN_IF_FAILED(InitPropVariantFromString(title, propVariant.addressof()));
            RETURN_IF_FAILED(propertyStore->SetValue(PKEY_Title, propVariant));
            RETURN_IF_FAILED(propertyStore->Commit());
        }
        return collection->AddObject(shellLink.get());
    }
    std::wstring string_to_w(const std::string& str)
    {
        return std::wstring(str.begin(), str.end());
    }
    // https://windhawk.net/mods/taskbar-icon-size
    // Icon size: 24x24, taskbar height: 48 (Windows 11 default)
    // width each Icon edge = 44, edge of first = 10
    POINT GetTaskBarPoint(u_short nth)
    {
        APPBARDATA abd = {sizeof(APPBARDATA)};
        UINT position = SHAppBarMessage(ABM_GETTASKBARPOS, &abd);
        if (abd.uEdge == ABE_TOP || abd.uEdge == ABE_BOTTOM)
            return POINT{abd.rc.left + (10 + 22 + 44 * nth), abd.rc.top + 24};
        else
            return POINT{abd.rc.left + 24, abd.rc.top + (10 + 22 + 44 * nth)};
    }    
};

int WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Engine eng;
    std::ifstream f("config.json");
    if (f.is_open())
    {
        json data = json::parse(f,nullptr,false);
        if(!data.is_null())
        {
            ns::Config config = data.get<ns::Config>();
            if(config.action.compare("show")==0){
                eng.ShowMenu(config.iconInTaskbar);
                return 0;
            }            
            HRESULT hr = eng.CreateJumpList(config);
            if(S_OK == hr){
                MessageBox(nullptr,L"Successfully updated shortcuts",L"Shortcuts",MB_OK|MB_ICONINFORMATION);
            }
            else{
                MessageBox(nullptr,L"Failed updated shortcuts",L"Shortcuts",MB_OK|MB_ICONERROR);
             }
        }
    }
    return 0;
}