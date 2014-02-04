i=1
base=512*9-1
puts '<data xmlns="http://schemas.malighting.de/grandma2/xml/MA">'
File.open(ARGV[0]).each_line do |line|
  line.chomp!
  a = line.split(/\t/)
  if a.size > 1 then
    puts "<key index='#{i}' scan='#{a[0]}' assign='#{a[1].to_i+base}' />"
    i = i + 1
  end
end
(76..99).each do |bump|
    puts "<bump index='#{i}' scan='#{100+bump-76}' assign='#{bump+base}' />"
    i = i + 1
end
(1..75).each do |fader|
    puts "<fader index='#{i}' scan='#{fader-1}' assign='#{fader+base}' />"
    i = i + 1
end
puts "</data>"
