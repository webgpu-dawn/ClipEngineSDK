RenderPipeline pipeline_ CreateRenderPipeline
│
├─ PipelineLayout pipelineLayout
│    └─ BindGroupLayout bind_group_layout_
│         ├─ binding 0 → Sampler (sampler_)
│         └─ binding 1 → Texture (texture_)
│
├─ VertexState vertexState
│    ├─ ShaderModule module (vs entry)
│    └─ VertexBufferLayout vbLayout
│         ├─ Attribute 0 → pos (Float32x2) @location(0)
│         └─ Attribute 1 → uv  (Float32x2) @location(1)
│
└─ FragmentState fragmentState
     ├─ ShaderModule module (fs entry)
     └─ ColorTargetState target
          └─ format = surface_texture_fmt_