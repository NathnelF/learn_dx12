# List the steps needed to get a triangle on the screen

1. Set up D3D12 context
	1. Enabel the D3D12 debug layer
	1. Create a DXGI factory -> this is our link into the hardware layer
	2. Enumerate the DXGI adapters -> these are our available GPUs
	3. Pick the most optimal gpu
	4. Create a D3D12 device using our gpu
	5. Create a D3D12 queue tied to our gpu
	6. Create a DXGI swapchain handle -> this stays alive for the whole program we just resize it on window resize
	7. Create our window with SDL and pass it's hwnd to the swapchain
	8. Create a D3D12 command allocator -> this is the memory backing for our command lists
2. Create DXGI swapchain resources
	1. Create our D3D12 descriptor heap for RTV resources (swapchain image)
	2. Get our cpu side rtv handle for descriptor heap
	3. create our ID3D12 resources for swapchain images
	4. Tie the handles to the image buffers
	5. 
3. Write shaders
4. Create pipeline state object with compiled shaders
5. Open command list and record draw command
6. Submit draw command(s) to queue 


A descriptor can contain
	1. GPU virtual addresses -> where to find this resource
	2. Resource Formats specifying how the resource should be interpreted -> 
	3. Additional metadata