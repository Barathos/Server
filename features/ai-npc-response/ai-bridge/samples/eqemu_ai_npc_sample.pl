# Stage-only sample for D:\EQEmu\Testbed\ai-bridge\samples.
# Do not copy over a live quest script without backing it up first.
#
# Phase 1 behavior: synchronous HTTP call from EVENT_SAY with a short timeout.
# This proves the data path but can briefly block the zone process. Before live
# use, prefer queue/poll behavior from EVENT_SAY + EVENT_TIMER.

BEGIN {
    unshift @INC, "D:/EQEmu/Testbed/server/perl/vendor/lib";
    unshift @INC, "D:/EQEmu/Testbed/server/perl/site/lib";
    unshift @INC, "D:/EQEmu/Testbed/server/perl/lib";
}

use HTTP::Tiny;
use JSON::PP;

our ($client, $name, $npc, $npcid, $text, $zonesn);

my $AI_BRIDGE_URL = "http://127.0.0.1:18080/eqemu/npc-chat";
my $AI_TIMEOUT_SECONDS = 4;

sub EVENT_SAY {
    my $player_message = defined $text ? $text : "";
    $player_message =~ s/^\s+|\s+$//g;
    return if $player_message eq "";

    my $npc_name = "Unknown NPC";
    if (defined $npc) {
        my $clean_name = eval { $npc->GetCleanName() };
        $npc_name = $clean_name if defined $clean_name && $clean_name ne "";
    }

    my $payload = {
        npc_id => defined $npcid ? $npcid : 0,
        npc_name => $npc_name,
        zone_short_name => defined $zonesn ? $zonesn : "unknown",
        player_name => defined $name ? $name : "Adventurer",
        player_message => $player_message,
        recent_context => []
    };

    my $response = ai_npc_bridge_chat($payload);
    if ($response) {
        quest::say($response);
    } else {
        quest::say("Give me a moment, friend. My thoughts are elsewhere.");
    }
}

sub ai_npc_bridge_chat {
    my ($payload) = @_;

    my $json = eval { JSON::PP->new->ascii->canonical->encode($payload) };
    return undef if !$json;

    my $http = HTTP::Tiny->new(
        timeout => $AI_TIMEOUT_SECONDS,
        agent => "eqemu-ai-npc-prototype/0.1"
    );

    my $res = eval {
        $http->post(
            $AI_BRIDGE_URL,
            {
                headers => { "content-type" => "application/json" },
                content => $json
            }
        );
    };
    return undef if !$res || !$res->{success};

    my $body = eval { JSON::PP->new->decode($res->{content}) };
    return undef if !$body || !$body->{response};

    my $speech = $body->{response};
    $speech =~ s/[\r\n\t]+/ /g;
    $speech =~ s/\s+/ /g;
    $speech =~ s/^\s+|\s+$//g;
    return substr($speech, 0, 420);
}
