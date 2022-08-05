# Good Practices:

## C++ Usage

### Prevent Crashes

😣  Don\'t:
```c++
jtl::lent_ptr<QuestEntryWidget> test = jtl::dynamic_lent_cast<QuestEntryWidget>(child);
JTL_ASSERT(test != nullptr, "Container does not hold QuestEntryWidgetEP");
if (test->IsCompleted())
{
    ...
}

```

👍  Do:
```c++
jtl::lent_ptr<QuestEntryWidget> test = jtl::dynamic_lent_cast<QuestEntryWidget>(child);
JTL_ASSERT(test != nullptr, "Container does not hold QuestEntryWidgetEP");
if (test != nullptr && test->IsCompleted())
{
    ...
}
```

Prevent trivial crashes if possible

----------------------------------------------------------------------------------

### [insert_or_assign](https://en.cppreference.com/w/cpp/container/map/insert_or_assign)

😣  Don\'t:
```c++
auto it = m_playersData.find(i_user);
if (it == m_playersData.end())
{
    m_playersData.emplace(i_user, OnlineRacerData(i_user->GetName(), i_user->GetAvatar(), i_user->GetTotalRank()));
}
else
{
    it->second = OnlineRacerData(i_user->GetName(), i_user->GetAvatar(), i_user->GetTotalRank());
}
```

👍  Do:
```c++
m_playersData.insert_or_assign(i_user, OnlineRacerData(i_user->GetName(), i_user->GetAvatar(), i_user->GetTotalRank()));
```

----------------------------------------------------------------------------------

### [std::vector::emplace_back](https://en.cppreference.com/w/cpp/container/vector/emplace_back) returns a reference

😣  Don\'t:
```c++
jtl::unique_ref<Foo> foo = Create();
jtl::lent_ref<Foo> fooRef = foo;
m_vector.emplace_back(std::move(foo));
...
fooRef->SomeUsageAfterEmplacing();
```

👍  Do:
```c++
const jtl::lent_ref<Foo> foo = m_vector.emplace_back(std::move(foo));
...
fooRef->SomeUsageAfterEmplacing();
```

If the following statement is too complex, don't hesitate to split it more the point here is that emplace_back returns a reference to the inserted element.

----------------------------------------------------------------------------------


### Missing override in polymorphic objects
😣  Don\'t:
```c++
class Widget : public IWidget
{
public:
    ~Widget();
};
```

👍  Do:
```c++
class Widget : public IWidget
{
public:
    ~Widget() override;
};
```
----------------------------------------------------------------------------------

### protect switches for future changes
----------------------------------------------------------------------------------
😣  Don\'t:
```c++
AnotherValue Convert(Value i_value)
{
    switch(value)
    {
        case Value::Something: return AnotherValue::A;
        case Value::Anytime: return AnotherValue::Z;
        case Value::AnotherCase: return AnotherValue::Whatever;
        default:
            JTL_ASSERT(false, "Invalid Case");
            return AnotherValue::A;
    }
}
```

👍  Do:
```c++
AnotherValue Convert(Value i_value)
{
    switch(value)
    {
        case Value::Something: return AnotherValue::A;
        case Value::Anytime: return AnotherValue::Z;
        case Value::AnotherCase: return AnotherValue::Whatever;
        // default:
        // Do not define default if want to handle all the cases, that way, the compiler won't compile if anyone adds a new type

        // Sometimes, if you use jtl enums, you will have to treat an special autogenerate type
        case Value::COUNT: break;
    }

    JTL_FAIL("Invalid Case");
    return AnotherValue::A;
}
```
----------------------------------------------------------------------------------
### auto never references to a reference
----------------------------------------------------------------------------------

😣  Don\'t:
```c++
for (auto data : m_container)
{
    ...
}
```

👍  Do:
```c++
for (const auto& data : m_container)
{
    ...
}
```

### Use nested-namespaces
----------------------------------------------------------------------------------

😣  Don\'t:
```c++
namespace game
{
namespace gui
{
    
}
}
```

👍  Do:
```c++
namespace game::gui
{
    
}
```

### Avoid redundant namespaces
----------------------------------------------------------------------------------

😣  Don\'t:
```c++
namespace game::clientlogic
{
game::DriverId Calculate(const game::clientlogic::SomeData& i_data, game::SupporterId i_id);
}
```

👍  Do:
```c++
namespace game::clientlogic
{
DriverId Calculate(const SomeData& i_data, SupporterId i_id);
}
```

If you don't need extra namespace qualifiers, don't put it. That should make the code cleaner and more readable.

### it referes to iterator
----------------------------------------------------------------------------------

😣  Don\'t:
```c++
for (const auto& it : vectorOfSomething)
{
    ...
}
```

👍  Do:
```c++
for (const auto& something : container)
{
    ....
}
```

### erase return a count
----------------------------------------------------------------------------------

😣  Don\'t:
```c++
auto it = m_playersData.find(i_user);
JTL_ASSERT(it != m_playersData.end());
if (it != m_playersData.end())
{
    m_playersData.erase(i_user);
}
```

👍  Do:
```c++
const size_t eraseCount = m_playersData.erase(i_user);
JTL_ASSERT(eraseCount == 1, "We should have knowledge of that user. What happened?");
```

### Avoid const-ref values
----------------------------------------------------------------------------------v

😣  Don\'t:
```c++
struct Data
{
    const Something& something; 
};
```

👍  Do:
```c++
struct Data
{
    Something something;    
};
```

👍  Do:
```c++
struct Data
{
    jtl::lent_ref<Something> something; 
};
```

Const-ref can dangle quick and silently.

### Initialize trivial types
----------------------------------------------------------------------------------

😣  Don\'t:
```c++
struct KartshowcaseStepDef
{
    uint32_t autoskipShotCount;
};
```

👍  Do:
```c++
struct KartshowcaseStepDef
{
    uint32_t autoskipShotCount = 0;
};
```
----------------------------------------------------------------------------------
### Overcomplicated invalid states
😣  Don\'t:
```c++
userProfile = online::PublicUserProfile::CreateFrom(jtl::string("Unknown User"), jtl::none, jtl::none, jtl::none, jtl::none, jtl::none, jtl::none);
```

👍  Do:
```c++
userProfile = online::PublicUserProfile::Invalid();
```

There are several solutions here, but if you encounter a case like that, try to think of a more readable approach if possible

----------------------------------------------------------------------------------
## JTL

### Formatting Errors
😣  Don\'t:
```c++
JTL_ASSERT(mappingResult == jtl::success, "{}", mappingResult.error().what());
```

😣  Don\'t:
```c++
JTL_ASSERT(mappingResult == jtl::success, mappingResult.error().what());
```

😣  Don\'t:
```c++
JTL_ASSERT(mappingResult == jtl::success);
```


👍  Do:
```c++
JTL_ASSERT(mappingResult == jtl::success, "{}", mappingResult.error());
```

----------------------------------------------------------------------------------

### JTL_ASSERT vs JTL_FAIL
😣  Don\'t:
```c++
JTL_ASSERT(false, "{}", mappingResult.error());
```

👍  Do:
```c++
JTL_FAIL("{}", mappingResult.error());
```

### jtl::typed_number are iterable
----------------------------------------------------------------------------------
😣  Don\'t:
```c++
for (uint32_t level = 0; level <= m_logic->GetCollections()->GetMaxCollectionLevel().value(); ++level)
```

👍  Do:
```c++
for (CollectionsLevel level = CollectionsLevel::zero(); level <= m_logic->GetCollections()->GetMaxCollectionLevel(); ++level)
```

----------------------------------------------------------------------------------
### Don\'t lose error information
😣  Don\'t:
```c++
auto rewardWidgetInitializeResult = rewardWidget->Initialize();
JTL_ASSERT(rewardWidgetInitializeResult == jtl::success, rewardWidgetInitializeResult.error());
if (rewardWidgetInitializeResult != jtl::success)
{
    return jtl::make_error<gin::CustomInitializationError>(gin::CustomInitializationErrorCode::GinMappingError,
                                                                       rewardWidgetInitializeResult.error().what());
```

👍  Do:
```c++
auto rewardWidgetInitializeResult = rewardWidget->Initialize();
JTL_ASSERT(rewardWidgetInitializeResult == jtl::success, rewardWidgetInitializeResult.error());
if (rewardWidgetInitializeResult != jtl::success)
{
    return rewardWidgetInitializeResult.error();
}
```

If the result type is the same, there is no need to lose the code/message from the original error

----------------------------------------------------------------------------------
## GUI

### Use helpers::SetLocalizedText
😣  Don\'t:
```c++
m_inProgressValue->GetLocalizationData()->SetValue("value", m_progress.value());
m_inProgressValue->GetLocalizationData()->SetValue("totalvalue", m_targetValue);
m_completeValue->GetLocalizationData()->SetValue("value", m_progress.value());
```

👍  Do:
```c++
helpers::SetLocalizedText(*m_inProgressValue, "value", m_progress.value());
helpers::SetLocalizedText(*m_inProgressValue, "value", m_progress.value());
helpers::SetLocalizedText(*m_inProgressValue, "totalvalue", m_targetValue);
helpers::SetLocalizedText(*m_completeValue, "value", m_progress.value());
```

This will control that situation better:
1. GetLocalizationData can be nullptr -> SetLocalizedText will assert and prevent the crash
2. SetValue returns a result (as enum) -> SetLocalizedText will assert if SetValue failed

----------------------------------------------------------------------------------

### Proper clonning
😣  Don\'t:
```c++
jtl::unique_ref<gin::IWidget> CollectionLevelRewardsWidget::CloneWidget() const
{
    return jtl::make_unique<CollectionLevelRewardsWidget>(*this);
}
```
👍  Do:
```c++
jtl::unique_ref<gin::IWidget> CollectionLevelRewardsWidget::CloneWidget() const
{
    jtl::unique_ref<CollectionLevelRewardsWidget> clonedWidget = jtl::make_unique<CollectionLevelRewardsWidget>(*this);
    const auto result = clonedWidget->SetupGUI(*clonedWidget->GetRootTemplate());
    JTL_ASSERT(result == jtl::success, "Failed to clone CollectionLevelRewardsWidget: {}", result.error());
    return std::move(clonedWidget);
}

```

----------------------------------------------------------------------------------

## Forward Declarations
👍  Do:

That's a HUGE topic, basically, don't add includes in the header just to make it compile. Add includes in headers because you KNOW they are needed.

----------------------------------------------------------------------------------

## Guidelines

😣  Don\'t:

Do not define protected member variables
https://git.gameloft.org/jet_engine/jet_engine/-/blob/master/docs/codingGuideline.md#do-not-define-protected-member-variables

----------------------------------------------------------------------------------

👍  Do:

Always use // instead of block commenting multiple lines.
This way when you search for something you will see immediately if it's
commented or not.

----------------------------------------------------------------------------------

👍  Do:

Order of declarations
👉 The order of declaration within a class body should be as follows:
By visibility group:

Public
Protected
Private

Then, within each group:

Typedefs
Constructors
Destructor
Operators
Member functions - Grouped by semantic
Member variables - Grouped by semantic


----------------------------------------------------------------------------------

