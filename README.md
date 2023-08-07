# ae-plugins
Testing plugins using Adobe AE SDK.
## Configurations and Set-Up
We want to use either XCode or AppCode to run this code, as it provides a simple workflow with effective debugging features.

First, in XCode, navigate to File -> Project Settings. You want to change your Derived Data location to match what's shown below.
The path should be your plug-in's folder within the AE application, such as 
```/Applications/Adobe After Effects 2023/Plug-ins/Shade```.

<img width="509" alt="Screen Shot 2022-12-22 at 4 56 27 PM" src="https://user-images.githubusercontent.com/75495429/209232834-3f01bdb5-b501-4d55-8451-c1746b6c99e4.png">

Next, make sure to edit the scheme's executable to the actual After Effects application itself. When you run the code, it should automatically boot up a new AE application.

<img width="563" alt="Screen Shot 2022-12-22 at 5 05 52 PM" src="https://user-images.githubusercontent.com/75495429/209233867-e12012d5-aa1d-42d8-855b-8b4016a4d990.png">

## Notes
You might get tons of warnings when building the example projects (like 150+), but it should be fine as long as it runs properly.
