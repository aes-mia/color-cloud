//
// initialize
//

var canvas = document.querySelector("#c");
var gl = canvas.getContext("webgl");
if (!gl) console.log("no webgl");

var programInfo = webglUtils.createProgramInfo(gl, ["vertex-shader-2d", "fragment-shader-2d"]);

var arrays = {
    position: {numComponents: 2, data:
    [
        0,   0,
        0, 150,
        60,   0,
        0, 150, 
        60, 150,
        60,   0,
    ]
    },
    texcoord: {numComponents: 2, data:
    [
        0.3, 0.0,
        0.3, 0.5,
        0.5, 0.0,
        0.3, 0.5,
        0.5, 0.5,
        0.5, 0.0,
    ]}
};
var bufferInfo = webglUtils.createBufferInfoFromArrays(gl, arrays);

// Create a texture.
var texture = gl.createTexture();
gl.bindTexture(gl.TEXTURE_2D, texture);
// Fill the texture with a 1x1 blue pixel.
gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, 1, 1, 0, gl.RGBA, gl.UNSIGNED_BYTE,
              new Uint8Array([0, 0, 255, 255]));
// Asynchronously load an image
var image = new Image();
image.src = "f-texture.png";
image.addEventListener('load', function() {
  // Now that the image has loaded make copy it to the texture.
  gl.bindTexture(gl.TEXTURE_2D, texture);
  gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA,gl.UNSIGNED_BYTE, image);
  //gl.generateMipmap(gl.TEXTURE_2D);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
   gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
   gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
});

requestAnimationFrame(drawScene);

//
// rendering
//
function drawScene(now){
    webglUtils.resizeCanvasToDisplaySize(gl.canvas);
    gl.viewport(0, 0, gl.canvas.width, gl.canvas.height);

    gl.enable(gl.CULL_FACE);
    gl.enable(gl.DEPTH_TEST);

    gl.clearColor(0.9, 0.6, 0.6, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    gl.useProgram(programInfo.program);

    webglUtils.setBuffersAndAttributes(gl, programInfo, bufferInfo);

    var projectionMatrix = m3.projection(gl.canvas.width, gl.canvas.height);

    var uniforms = {
        u_matrix: projectionMatrix,
        u_texture: texture
    };
    webglUtils.setUniforms(programInfo, uniforms);

    gl.drawArrays(gl.TRIANGLES, 0, bufferInfo.numElements);

    requestAnimationFrame(drawScene);
}
