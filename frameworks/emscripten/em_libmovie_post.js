var cwrap = AEMovie["cwrap"]
AEMovie["create_player"] = cwrap('em_create_player', 'number', ['string', 'number', 'number', 'number'])
AEMovie["create_movie_data"] = cwrap('em_create_movie_data', 'number', ['number', 'number'])
AEMovie["create_movie_composition"] = cwrap('em_create_movie_composition', 'number', ['number', 'number', 'string'])
AEMovie["play_movie_composition"] = cwrap('em_play_movie_composition', 'void', ['number', 'number'])
AEMovie["set_movie_composition_loop"] = cwrap('em_set_movie_composition_loop', 'void', ['number', 'number'])
AEMovie["update_movie_composition"] = cwrap('em_update_movie_composition', 'void', ['number', 'number', 'number'])
AEMovie["render_movie_composition"] = cwrap('em_render_movie_composition', 'void', ['number', 'number'])
AEMovie["delete_movie_data"] = cwrap('em_delete_movie_data', 'void', ['number', 'number'])
AEMovie["delete_player"] = cwrap('em_delete_player', 'void', ['number'])
AEMovie["utils_opengl_create_texture"] = cwrap('em_utils_opengl_create_texture', 'void', ['number', 'number', 'number', 'number'])
AEMovie["set_movie_composition_wm"] = cwrap('em_set_movie_composition_wm', 'void', ['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number'])

AEMovie["utils_opengl_initialize"] = function(canvas)
{    
    try 
    {
        var context = canvas.getContext("webgl");
        
        if( context == null )
        {
            context = canvas.getContext("experimental-webgl");
        }
     
        if( context == null) 
        {
            console.log("Could not initialize WebGL")
            
            return false
        }
        
        var webGLContextAttributes = {}
        webGLContextAttributes['majorVersion'] = 2;
        webGLContextAttributes['minorVersion'] = 0;
        
        var glctxhandle = AEMovie["GL"].registerContext(context, webGLContextAttributes);
        AEMovie["GL"].makeContextCurrent(glctxhandle)

        return context
    } 
    catch(e) 
    {
        console.log(e)
    }
    
    return false
}

AEMovie["utils_opengl_load_texure_from_image"] = function(imageloadercanvas, id, image)
{
    imageloadercanvas.setAttribute('width', '250px');
    imageloadercanvas.setAttribute('height', '200px');
    
    context = imageloadercanvas.getContext('2d');
    
    var width = 250
    var height = 200
    
    context.drawImage(image,0,0, image.width, image.height, 0, 0, width, height);
    
    var image_data = context.getImageData(0, 0, width, height);
    
    var ptr = AEMovie["utils_malloc_arraybuffer"](image_data.data)

    AEMovie["utils_opengl_create_texture"](id, width, height, ptr)
    
    AEMovie["utils_free_arraybuffer"](ptr);
}

AEMovie["utils_malloc_arraybuffer"] = function(arraybuffer)
{
    var numBytes = arraybuffer.byteLength;
        
    var ptr = AEMovie["_malloc"](numBytes);
    var heapBytes= new Uint8Array(AEMovie["HEAPU8"].buffer, ptr, numBytes);
    heapBytes.set(new Uint8Array(arraybuffer));
    
    var byteOffset = heapBytes.byteOffset
    
    return byteOffset
}

AEMovie["utils_free_arraybuffer"] = function(ptr)
{
    AEMovie["_free"](ptr);
}