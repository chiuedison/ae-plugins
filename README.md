# ae-plugins
Testing plugins using Adobe AE SDK.
## Configurations and Set-Up
We want to use either XCode or AppCode to run this code, as it provides a simple workflow with effective debugging features.

First, in XCode, navigate to File -> Project Settings. You want to change your Derived Data location to match what's shown below.
The path should be your plug-in's folder within the AE application, such as 
```/Applications/Adobe After Effects 2023/Plug-ins/test-plugin```.

Next, make sure to edit the scheme's executable to the actual After Effects application itself. When you run the code, it should automatically boot up a new AE application.

## Notes
You might get tons of warnings when building the example projects (like 150+), but it should be fine as long as it runs properly.
