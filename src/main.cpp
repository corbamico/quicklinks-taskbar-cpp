#include "pch.h"
using namespace winrt;
using namespace winrt::Windows::Foundation;
using json = nlohmann::json;

namespace ns
{
    struct Item
    {
        std::string title;
        std::string path;
        std::string args;
    };

    struct Config
    {
        std::vector<Item> items;
        u_short iconInTaskbar = 0;
    };

    void from_json(const json &j, Item &p)
    {
        j.at("title").get_to(p.title);
        j.at("path").get_to(p.path);
        j.at("args").get_to(p.args);
    }

    void from_json(const json &j, Config &p)
    {
        j.at("items").get_to(p.items);
        //j.at("iconInTaskbar").get_to(p.iconInTaskbar);
    }
}

class Engine
{
    winrt::com_ptr<IUIAutomation> _automation;
    winrt::com_ptr<IUIAutomationCondition> _condition;    
public:
    Engine()
    {
        winrt::init_apartment();
        _automation = try_create_instance<IUIAutomation>(guid_of<CUIAutomation>());
        _automation->CreatePropertyCondition(UIA_NamePropertyId, wil::make_variant_bstr(L"quicklinks"), _condition.put());

    }
    ~Engine() { winrt::uninit_apartment(); }
    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;
    Engine(Engine &&) = delete;
    Engine &operator=(Engine &&) = delete;

    static HRESULT CreateJumpList(const ns::Config &config)
    {
        auto customDestinationList = wil::CoCreateInstance<ICustomDestinationList, wil::err_returncode_policy>(CLSID_DestinationList);

        UINT maxSlots{};
        winrt::com_ptr<IObjectArray> removedItems;
        RETURN_IF_FAILED(customDestinationList->BeginList(&maxSlots, IID_PPV_ARGS(removedItems.put())));

        // Create a custom category
        auto objectCollection = wil::CoCreateInstance<IObjectCollection, wil::err_returncode_policy>(CLSID_EnumerableObjectCollection);

        // Add tasks to the custom category
        for (const auto &item : config.items)
        {
            AddTaskToCollection(objectCollection.get(),
                                string_to_w(item.title).c_str(),
                                string_to_w(item.path).c_str(),
                                string_to_w(item.args).c_str());
        }

        auto objectArray = objectCollection.try_query<IObjectArray>();
        HRESULT hr = customDestinationList->AddUserTasks(objectArray.get());
        customDestinationList->CommitList();
        return hr;
    }

    static bool ConfigChanged(const json &jsonConfig)
    {
        bool changed{true};
        size_t hash_in_file{0};
        std::ifstream iff{"quicklinks.hash"};

        std::hash<std::string> hasher;
        size_t hash_value = hasher(jsonConfig.at("items").dump());

        if (iff.is_open())
        {
            iff >> hash_in_file;
            iff.close();
            if (hash_value == hash_in_file)
            {
                changed = false;
            }
        }

        if (changed)
        {
            std::ofstream of{"quicklinks.hash"};
            of << hash_value;
            of.close();
        }
        return changed;
    }
    // void ShowContextMenu()
    // {
    //     winrt::com_ptr<IUIAutomationElement> window_element;
    //     winrt::com_ptr<IUIAutomationElement> button_element;

    //     try{
    //         auto window = FindWindowW(L"Shell_TrayWnd", nullptr);
    //         winrt::check_hresult(_automation->ElementFromHandle(window, window_element.put()));            
            
    //         winrt::check_hresult(window_element->FindFirst(TreeScope_Descendants, _condition.get(), button_element.put()));
    //         if (button_element)
    //         {
    //             button_element.as<IUIAutomationElement3>()->ShowContextMenu();
    //         }
    //     }
    //     catch (winrt::hresult_error &e){}
    //     catch (std::exception &e){}         
    // }
    void ShowMenuOnIcon()
    {
        winrt::com_ptr<IUIAutomationElement> window_element;
        winrt::com_ptr<IUIAutomationElement> button_element;

        try{
            auto window = FindWindowW(L"Shell_TrayWnd", nullptr);
            winrt::check_hresult(_automation->ElementFromHandle(window, window_element.put()));            
            
            winrt::check_hresult(window_element->FindFirst(TreeScope_Descendants, _condition.get(), button_element.put()));
            if (button_element)
            {
                POINT pt,ptCurrent;
                BOOL  clickable;                
                button_element->GetClickablePoint(&pt,&clickable);
                INPUT inputs[2];
                //
                int normalizedX = pt.x * 0xFFFF  / GetSystemMetrics(SM_CXSCREEN);
                int normalizedY = pt.y * 0xFFFF  / GetSystemMetrics(SM_CYSCREEN);

                GetCursorPos(&ptCurrent);

                //
                inputs[0].type = INPUT_MOUSE;
                inputs[0].mi.dx = normalizedX;
                inputs[0].mi.dy = normalizedY;
                inputs[0].mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_MOVE;

                //
                inputs[1].type = INPUT_MOUSE;
                inputs[1].mi.dx = normalizedX;
                inputs[1].mi.dy = normalizedY;
                inputs[1].mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTUP;

                //
                SendInput(2, inputs, sizeof(INPUT));

                SetCursorPos(ptCurrent.x, ptCurrent.y);
            }
        }
        catch (winrt::hresult_error &e){}
        catch (std::exception &e){}        
    }

private:
    // NOLINTNEXTLINE(readability-function-cognitive-complexity,bugprone-easily-swappable-parameters)
    static HRESULT AddTaskToCollection(IObjectCollection *collection, LPCWSTR title, LPCWSTR appPath, LPCWSTR args)
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
    static std::wstring string_to_w(const std::string &str)
    {
        return {str.begin(), str.end()};
    }
};

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    Engine eng;
    std::ifstream f("config.json");
    if (f.is_open())
    {
        json data = json::parse(f, nullptr, false);
        if (!data.is_null())
        {
            ns::Config config = data.get<ns::Config>();
            if (!Engine::ConfigChanged(data))
            {
                // if no changed, show context menu
                eng.ShowMenuOnIcon();
            }
            else
            {
                // if config changed, register it in JumpList
                HRESULT hr = Engine::CreateJumpList(config);
                if (S_OK == hr)
                {
                    MessageBox(nullptr, L"Successfully updated shortcuts", L"Shortcuts", MB_OK | MB_ICONINFORMATION);
                }
                else
                {
                    MessageBox(nullptr, L"Failed updated shortcuts", L"Shortcuts", MB_OK | MB_ICONERROR);
                }
            }
        }
    }
    return 0;
}