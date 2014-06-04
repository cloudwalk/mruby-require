MRuby::Gem::Specification.new('mruby-require') do |spec|
  spec.license = 'MIT'
  spec.authors = 'Internet Initiative Japan Inc.'

  ['mruby-array-ext', 'mruby-pax-io'].each do |v|
    add_dependency v
  end

  spec.cc.include_paths << "#{build.root}/src"
end

