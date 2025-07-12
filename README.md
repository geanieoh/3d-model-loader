## how
- parses vertex positions and UV coordinates into separate arrays
- parses face data, pairing each vertex index with its corresponding UV index
- constructs interleaved vertex attribute data (e.g., [x, y, z, u, v])
- builds an index array for use with an Element Buffer Object (EBO)

## why
- automate creation of VBO and EBO arrays for opengl rendering
- simplify rendering pipeline setup by converting .obj files to opengl ready buffers

<img width="1920" height="1080" alt="Screenshot (113)" src="https://github.com/user-attachments/assets/cc3f8364-91b4-40a3-8082-05b1ef4c5c7a" />
