function playhistory( position_history )
%PLAYHISTORY Play back generated position history
ph = position_history;

view(90, 0);

possize = size(ph);
if numel(possize) == 3
    num_frames = possize(end);
else
    num_frames = 1;
end
for i=1:num_frames
    p = ph(:,:,i);
    
    [az,el] = view;
    scatter3(p(1,1:possize(2)), p(2,1:possize(2)), p(3,1:possize(2)), 10);
    axis([-1 1 -1 1 -1 1]);
    view(az,el);
    drawnow;
end

end

