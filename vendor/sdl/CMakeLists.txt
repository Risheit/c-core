include(fetchcontent)

fetchcontent_declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/sdl.git 
  GIT_TAG release-3.2.x
)

fetchcontent_makeavailable(SDL3)
