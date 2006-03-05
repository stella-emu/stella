
package Btrees;
$VERSION=1.00;

require 5.000;
require Exporter;

=head1 NAME

    Btrees - Binary trees using the AVL balancing method.

=head1 SYNOPSIS

    # yes, do USE the package ...
    use Btrees;

    # no constructors

    # traverse a tree and invoke a function
    traverse( $tree, $func );

    # add a node in a balanced tree, rebalancing if required 
    ($tree, $node) = bal_tree_add( $tree, $val, $cmp )

=head1 DESCRIPTION

    Btrees uses the AVL balancing method, by G. M. Adelson-Velskii
    and E.M. Landis. Bit scavenging, as done in low level languages like
    C, is not used for height balancing since this is too expensive for
    an interpreter. Instead the actual height of each subtree is stored
    at each node. A null pointer has a height of zero. A leaf a height of
    1. A nonleaf a height of 1 greater than the height of its two children.

=head1 AUTHOR

 Ron Squiers (ron@broadcom.com). Adapted from "Mastering Algorithms with
 Perl" by Jon Orwant, Jarkko Hietaniemi & John Macdonald. Copyright
 1999 O'Reilly and Associates, Inc. All right reserved. ISBN: 1-56592-398-7

=cut

@ISA = qw(Exporter);
@EXPORT = qw( bal_tree_add label_tree );

#########################################
#
# Method: label_tree
#
# label_tree( $tree, $func );
#
sub label_tree {
    my $tree = shift or return undef;
    my $func = shift or return undef;

	$tree->{index} = 0;

    # Label all nodes with their respective indices
    sub label_node {
        my $tree = shift;

	    &$func( $tree );

		if (defined $tree->{left}) {
			$tree->{left}->{index}  = 2 * $tree->{index} + 1;
		}
		if (defined $tree->{right}) {
			$tree->{right}->{index} = 2 * $tree->{index} + 2;
		}
    }
    preorder_traverse( $tree, \&label_node );
}

#########################################
#
# Method: inorder_traverse
#
# Traverse $tree in order, calling $func() for each element.
#    in turn 
# inorder_traverse( $tree, $func );
#
sub inorder_traverse {
    my $tree = shift or return;	# skip undef pointers
    my $func = shift;

    inorder_traverse( $tree->{left}, $func );
    &$func( $tree );
    inorder_traverse( $tree->{right}, $func );
}

#########################################
#
# Method: preorder_traverse
#
# Traverse $tree in preorder form, calling $func() for each element.
#    in turn 
# preorder_traverse( $tree, $func );
#
sub preorder_traverse {
    my $tree = shift or return;	# skip undef pointers
    my $func = shift;

    &$func( $tree );
    preorder_traverse( $tree->{left}, $func );
    preorder_traverse( $tree->{right}, $func );
}

#########################################
#
# Method: bal_tree_add
#
# Search $tree looking for a node that has the value $val,
#    add it if it does not already exist. 
# If provided, $cmp compares values instead of <=>. 
#
# ($tree, $node) = bal_tree_add( $tree, $val, $cmp )
# the return values:
#    $tree points to the (possible new or changed) subtree that
#	has resulted from the add operation.
#    $node points to the (possibly new) node that contains $val
#
sub bal_tree_add {
    my( $tree, $val, $cmp) = @_;
    my $result;

    unless ( $tree ) {
	$result = { 
		left	=> undef,
		right	=> undef,
		val	    => $val,
		index   => -1,
		height	=> 1
	    };
	return( $result, $result );
    }

    my $relation = defined $cmp
	? $cmp->( $val, $tree->{val} )
	: $val <=> $tree->{val};

    ### Stop when the desired node if found.
    return ( $tree, $tree ) if $relation == 0;

    ### Add to the correct subtree.
    if( $relation < 0 ) {
	($tree->{left}, $result) =
	    bal_tree_add ( $tree->{left}, $val, $cmp );
    } else {
	($tree->{right}, $result) =
	    bal_tree_add ( $tree->{right}, $val, $cmp );
    }

    ### Make sure that this level is balanced, return the
    ###    (possibly changed) top and the (possibly new) selected node. 
    return ( balance_tree( $tree ), $result );
}

#########################################
#
# Method: balance_tree
#
# Balance a potentially out of balance tree 
#
# the return values:
#    $tree points to the balanced tree root
#
sub balance_tree {
    ### An empty tree is balanced already.
    my $tree = shift or return undef;

    ### An empty link is height 0.
    my $lh = defined $tree->{left} && $tree->{left}{height};
    my $rh = defined $tree->{right} && $tree->{right}{height};

    ### Rebalance if needed, return the (possibly changed) root.
    if ( $lh > 1+$rh ) {
	return swing_right( $tree );
    } elsif ( $lh+1 < $rh ) {
	return swing_left( $tree );
    } else {
	### Tree is either perfectly balanced or off by one.
	### Just fix its height.
	set_height( $tree );
	return $tree;
    }
} 

#########################################
#
# Method: set_height
#
# Set height of a node 
#
sub set_height {
    my $tree = shift;

    my $p;
    ### get heights, an undef node is height 0.
    my $lh = defined ( $p = $tree->{left}  ) && $p->{height};
    my $rh = defined ( $p = $tree->{right} ) && $p->{height};
    $tree->{height} = $lh < $rh ? $rh+1 : $lh+1;
}

#########################################
#
# Method: $tree = swing_left( $tree )
#
# Change        t       to      r      or       rl
#              / \             / \            /    \ 
#             l   r           t   rr         t      r
#                / \         / \            / \    / \
#               rl  rr      l   rl         l  rll rlr rr
#              /  \            / \
#            rll  rlr        rll rlr
#
# t and r must both exist.
# The second form is used if height of rl is greater than height of rr
# (since the form would then lead to the height of t at least 2 more
# than the height of rr).
#
# changing to the second form is done in two steps, with first a move_right(r)
# and then a move_left(t), so it goes:
#
# Change        t       to      t   and then to   rl
#              / \             / \              /    \ 
#             l   r           l   rl           t      r
#                / \             / \          / \    / \
#               rl  rr         rll  r        l  rll rlr rr
#              /  \                / \
#            rll  rlr            rlr  rr
#
sub swing_left {
    my $tree = shift;

    my $r = $tree->{right};	# must exist
    my $rl = $r->{left};	# might exist
    my $rr = $r->{right};	# might exist
    my $l = $tree->{left};	# might exist

    ### get heights, an undef node has height 0
    my $lh = $l && $l->{height} || 0;
    my $rlh = $rl && $rl->{height} || 0;
    my $rrh = $rr && $rr->{height} || 0;

    if ( $rlh > $rrh ) {
	$tree->{right} = move_right( $r );
    }

    return move_left( $tree );
}

# and the opposite swing

sub swing_right {
    my $tree = shift;

    my $l = $tree->{left};	# must exist
    my $lr = $l->{right};	# might exist
    my $ll = $l->{left};	# might exist
    my $r = $tree->{right};	# might exist 

    ### get heights, an undef node has height 0
    my $rh = $r && $r->{height} || 0;
    my $lrh = $lr && $lr->{height} || 0;
    my $llh = $ll && $ll->{height} || 0;

    if ( $lrh > $llh ) {
	$tree->{left} = move_left( $l );
    }

    return move_right( $tree );
}

#########################################
#
# Method: $tree = move_left( $tree )
#
# Change        t       to      r
#              / \             / \
#             l   r           t   rr
#                / \         / \
#               rl  rr      l   rl
#
# caller has determined that t and r both exist
#    (l can be undef, so can one of rl and rr)
#
sub move_left {
    my $tree = shift;
    my $r = $tree->{right};
    my $rl = $r->{left};

    $tree->{right} = $rl;
    $r->{left} = $tree;
    set_height( $tree );
    set_height( $r );
    return $r;
}

#########################################
#
# Method: $tree = move_right( $tree )
#
# Change        t       to      l
#              / \             / \
#             l   r          ll   t
#            / \                 / \
#           ll  lr             lr   r
#
# caller has determined that t and l both exist
#    (r can be undef, so can one of ll and lr)
#
sub move_right {
    my $tree = shift;
    my $l = $tree->{left};
    my $lr = $l->{right};

    $tree->{left} = $lr;
    $l->{right} = $tree;
    set_height( $tree );
    set_height( $l );
    return $l;
}

#########################################
# That's all folks ...
#########################################
#
1;  # so that use() returns true

