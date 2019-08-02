# Handlers:

```
/ -- index page
/login -- GET + POST
/register -- GET + POST
/tasks -- GET + POST
/masters -- GET
/profile?id=<UUID>
/profile/picture?id=<UUID>&size=100
```

# Scenarios:

## PUT

1. (with a p = 0.01) Register a new master
1. Register a new user
1. Load a list of masters
1. If empty, register a new master
1. Pick a master
1. Load master's profile + avatar
1. Create a task

## GET

1. Login as a user, check flag existence
1. Login as a master, check flag existence

# Internal composure:

* CookieStorage
  * Create(ci *CookieInfo) (string, error)
  * Load(cookie string) (*CookieInfo, error)
* UserStorage
  * Register(ui UserInfo) error
  * Login(username string, password string) (UserInfo, error)
  * GetPublicProfileInfo(id UUID) (ProfileInfo, error)
* TasksStorage
  * CreateTask(TaskInfo) error
  * RetrieveTasksFromUser(id UUID) ([]TaskInfo, error)
  * RetrieveTasksToUser(id UUID) ([]TaskInfo, error)
* AvatarGenerator
  * GenerateAvatar(id UUID, w io.Writer) error

User Info in Cookies:
  - AdditionalInfo
    - Username

cookie = id + encrypt(hash + serialize(user_info))